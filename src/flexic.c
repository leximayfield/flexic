//
// FlexiC - A standalone FlexBuffer reader/writer in C
//
// (C) 2025 Lexi Mayfield
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "flexic.h"

#include <stdlib.h>

#include <string.h>

/******************************************************************************/

#define UNPACK_TYPE(packed) ((flexi_type_e)(packed >> 2))
#define UNPACK_WIDTH(packed) ((flexi_width_e)(packed & 0x03))
#define WIDTH_TO_BYTES(width) ((int)(1 << width))

/******************************************************************************/

static bool type_is_int(flexi_type_e type)
{
    if (type == FLEXI_TYPE_INT || type == FLEXI_TYPE_INDIRECT_INT)
    {
        return true;
    }
    return false;
}

/******************************************************************************/

static bool type_is_uint(flexi_type_e type)
{
    if (type == FLEXI_TYPE_UINT || type == FLEXI_TYPE_INDIRECT_UINT)
    {
        return true;
    }
    return false;
}

/******************************************************************************/

static bool type_is_anyint(flexi_type_e type)
{
    switch (type)
    {
    case FLEXI_TYPE_INT:
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_INT:
    case FLEXI_TYPE_INDIRECT_UINT: return true;
    }
    return false;
}

/******************************************************************************/

static bool type_is_float(flexi_type_e type)
{
    if (type == FLEXI_TYPE_FLOAT || type == FLEXI_TYPE_INDIRECT_FLOAT)
    {
        return true;
    }
    return false;
}

/******************************************************************************/

static bool type_is_direct(flexi_type_e type)
{
    if (type >= FLEXI_TYPE_NULL && type <= FLEXI_TYPE_FLOAT)
    {
        return true;
    }
    else if (type == FLEXI_TYPE_BOOL)
    {
        return true;
    }
    return false;
}

/******************************************************************************/

static bool type_is_vector(flexi_type_e type)
{
    if (type >= FLEXI_TYPE_VECTOR && type <= FLEXI_TYPE_VECTOR_FLOAT4)
    {
        return true;
    }
    else if (type == FLEXI_TYPE_VECTOR_BOOL)
    {
        return true;
    }
    return false;
}

/******************************************************************************/

static bool load_sint_by_width(int64_t *dst, const char *src, int width)
{
    switch (width)
    {
    case 1: {
        int8_t v;
        memcpy(&v, src, 1);
        *dst = v;
        return true;
    }
    case 2: {
        int16_t v;
        memcpy(&v, src, 2);
        *dst = v;
        return true;
    }
    case 4: {
        int32_t v;
        memcpy(&v, src, 4);
        *dst = v;
        return true;
    }
    case 8: {
        int64_t v;
        memcpy(&v, src, 8);
        *dst = v;
        return true;
    }
    }
    return false;
}

/******************************************************************************/

static bool load_uint_by_width(uint64_t *dst, const char *src, int width)
{
    switch (width)
    {
    case 1: {
        uint8_t v;
        memcpy(&v, src, 1);
        *dst = v;
        return true;
    }
    case 2: {
        uint16_t v;
        memcpy(&v, src, 2);
        *dst = v;
        return true;
    }
    case 4: {
        uint32_t v;
        memcpy(&v, src, 4);
        *dst = v;
        return true;
    }
    case 8: {
        uint64_t v;
        memcpy(&v, src, 8);
        *dst = v;
        return true;
    }
    }
    return false;
}

/******************************************************************************/

static bool load_float_by_width(double *dst, const char *src, int width)
{
    switch (width)
    {
    case 4: {
        float v;
        memcpy(&v, src, 4);
        *dst = v;
        return true;
    }
    case 8: {
        double v;
        memcpy(&v, src, 8);
        *dst = v;
        return true;
    }
    }
    return false;
}

/******************************************************************************/

static bool cursor_resolve_offset(const char **dest, const flexi_buffer_s *buffer, const char *src, uint64_t offset)
{
    if (offset > PTRDIFF_MAX)
    {
        return false;
    }

    ptrdiff_t move = (ptrdiff_t)offset;
    ptrdiff_t max_move = src - buffer->data;
    if (move > max_move)
    {
        return false;
    }

    *dest = src - move;
    return true;
}

/******************************************************************************/

static bool cursor_get_len(const flexi_cursor_s *cursor, size_t *len)
{
    uint64_t v;
    if (!load_uint_by_width(&v, cursor->cursor - cursor->stride, cursor->stride))
    {
        return false;
    }

    *len = (size_t)v;
    return true;
}

/******************************************************************************/

static bool cursor_vector_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed)
{
    size_t len = 0;
    if (!cursor_get_len(cursor, &len))
    {
        return false;
    }

    const flexi_packed_t *p = (const flexi_packed_t *)cursor->cursor;
    *packed = p + len * cursor->stride;
    return true;
}

/******************************************************************************/

static bool cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index, flexi_cursor_s *dest)
{
    const flexi_packed_t *types = NULL;
    if (!cursor_vector_types(cursor, &types))
    {
        return false;
    }

    flexi_type_e type = UNPACK_TYPE(types[index]);
    if (type_is_direct(type))
    {
        // No need to resolve an offset, we're pretty much done.
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + (index * (size_t)cursor->stride);
        dest->type = type;
        dest->stride = cursor->stride;
        return true;
    }

    uint64_t offset = 0;
    const char *offset_ptr = cursor->cursor + (index * (size_t)cursor->stride);
    if (!load_uint_by_width(&offset, offset_ptr, cursor->stride))
    {
        return false;
    }

    if (!cursor_resolve_offset(&dest->cursor, cursor->buffer, offset_ptr, offset))
    {
        return false;
    }

    dest->buffer = cursor->buffer;
    dest->type = type;
    dest->stride = WIDTH_TO_BYTES(UNPACK_WIDTH(types[index]));
    return true;
}

/******************************************************************************/

static bool cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index, const char **str)
{
    // Figure out offset and width of keys vector.
    uint64_t keys_offset, keys_width;
    if (!load_uint_by_width(&keys_offset, cursor->cursor - (cursor->stride * 3), cursor->stride))
    {
        return false;
    }

    if (!load_uint_by_width(&keys_width, cursor->cursor - (cursor->stride * 2), cursor->stride))
    {
        return false;
    }

    // Seek the keys base.
    const char *keys_base = cursor->cursor - ((3 * cursor->stride) + keys_offset);

    // Resolve the key.
    uint64_t offset;
    const char *offset_ptr = keys_base + (index * (size_t)keys_width);
    if (!load_uint_by_width(&offset, offset_ptr, (int)keys_width))
    {
        return false;
    }

    return cursor_resolve_offset(str, cursor->buffer, offset_ptr, offset);
}

/******************************************************************************/

static bool cursor_seek_map_key_linear(const flexi_cursor_s *cursor, const char *key, flexi_cursor_s *dest, size_t len)
{
    // Linear search.
    for (size_t i = 0; i < len; i++)
    {
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, i, &cmp))
        {
            return false;
        }

        if (!strcmp(cmp, key))
        {
            return cursor_seek_vector_index(cursor, i, dest);
        }
    }

    return false;
}

/******************************************************************************/

static bool cursor_seek_map_key_bsearch(const flexi_cursor_s *cursor, const char *key, flexi_cursor_s *dest, size_t len)
{
    size_t left = 0;
    size_t right = len - 1;
    while (left <= right)
    {
        size_t i = left + ((right - left) / 2);
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, i, &cmp))
        {
            return false;
        }

        int res = strcmp(cmp, key);
        if (res == 0)
        {
            return cursor_seek_vector_index(cursor, i, dest);
        }

        if (res < 0)
        {
            left = i + 1;
        }
        else
        {
            right = i - 1;
        }
    }

    return false;
}

/******************************************************************************/

flexi_buffer_s flexi_make_buffer(const void *buffer, size_t len)
{
    flexi_buffer_s rvo;
    rvo.data = (const char *)buffer;
    rvo.length = (ptrdiff_t)len;
    return rvo;
}

/******************************************************************************/

bool flexi_buffer_open(const flexi_buffer_s *buffer, flexi_cursor_s *cursor)
{
    if (buffer->length < 3 || buffer->length > PTRDIFF_MAX)
    {
        // Shortest length we can discard without checking.
        return false;
    }

    // Width of root object.
    cursor->buffer = buffer;
    cursor->cursor = buffer->data + buffer->length - 1;
    uint8_t root_bytes = *(const uint8_t *)(cursor->cursor);
    if (buffer->length < root_bytes + 2)
    {
        return false;
    }

    // Obtain the packed type.
    cursor->cursor -= 1;
    flexi_packed_t packed = *(const uint8_t *)(cursor->cursor);

    // Point at the root object.
    flexi_type_e type = UNPACK_TYPE(packed);
    cursor->cursor -= root_bytes;
    if (type_is_direct(type))
    {
        // No need to resolve an offset, we're done.
        cursor->type = type;
        cursor->stride = root_bytes;
        return true;
    }

    // We're pointing at an offset, resolve it.
    uint64_t offset;
    if (!load_uint_by_width(&offset, cursor->cursor, root_bytes))
    {
        return false;
    }

    const char *dest = NULL;
    if (!cursor_resolve_offset(&dest, cursor->buffer, cursor->cursor, offset))
    {
        return false;
    }

    cursor->cursor = dest;
    cursor->type = type;
    cursor->stride = WIDTH_TO_BYTES(UNPACK_WIDTH(packed));
    return true;
}

/******************************************************************************/

flexi_type_e flexi_cursor_type(const flexi_cursor_s *cursor)
{
    return cursor->type;
}

/******************************************************************************/

int flexi_cursor_stride(const flexi_cursor_s *cursor)
{
    return cursor->stride;
}

/******************************************************************************/

bool flexi_cursor_length(const flexi_cursor_s *cursor, size_t *len)
{
    switch (cursor->type)
    {
    case FLEXI_TYPE_STRING:
    case FLEXI_TYPE_MAP:
    case FLEXI_TYPE_VECTOR:
    case FLEXI_TYPE_VECTOR_INT:
    case FLEXI_TYPE_VECTOR_UINT:
    case FLEXI_TYPE_VECTOR_FLOAT:
    case FLEXI_TYPE_BLOB:
    case FLEXI_TYPE_VECTOR_BOOL: return cursor_get_len(cursor, len);
    case FLEXI_TYPE_VECTOR_INT2:
    case FLEXI_TYPE_VECTOR_UINT2:
    case FLEXI_TYPE_VECTOR_FLOAT2: *len = 2; return true;
    case FLEXI_TYPE_VECTOR_INT3:
    case FLEXI_TYPE_VECTOR_UINT3:
    case FLEXI_TYPE_VECTOR_FLOAT3: *len = 3; return true;
    case FLEXI_TYPE_VECTOR_INT4:
    case FLEXI_TYPE_VECTOR_UINT4:
    case FLEXI_TYPE_VECTOR_FLOAT4: *len = 4; return true;
    }

    return false;
}

/******************************************************************************/

bool flexi_cursor_bool(const flexi_cursor_s *cursor, bool *v)
{
    if (type_is_anyint(cursor->type))
    {
        uint64_t u;
        if (!load_uint_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = (bool)u;
        return true;
    }
    else if (cursor->type == FLEXI_TYPE_FLOAT)
    {
        double u;
        if (!load_float_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = (bool)u;
        return true;
    }
    else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1)
    {
        *v = *(const bool *)(cursor->cursor);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_int(const flexi_cursor_s *cursor, int64_t *v)
{
    if (type_is_anyint(cursor->type))
    {
        int64_t u;
        if (!load_sint_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = u;
        return true;
    }
    else if (type_is_float(cursor->type))
    {
        double u;
        if (!load_float_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = (int64_t)u;
        return true;
    }
    else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1)
    {
        *v = *(const bool *)(cursor->cursor);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_uint(const flexi_cursor_s *cursor, uint64_t *v)
{
    if (type_is_anyint(cursor->type))
    {
        uint64_t u;
        if (!load_uint_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = u;
        return true;
    }
    else if (type_is_float(cursor->type))
    {
        double u;
        if (!load_float_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = (uint64_t)u;
        return true;
    }
    else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1)
    {
        *v = *(const bool *)(cursor->cursor);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_float(const flexi_cursor_s *cursor, double *v)
{
    if (type_is_int(cursor->type))
    {
        int64_t u;
        if (!load_sint_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = (double)u;
        return true;
    }
    else if (type_is_uint(cursor->type))
    {
        uint64_t u;
        if (!load_uint_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = (double)u;
        return true;
    }
    else if (type_is_float(cursor->type))
    {
        double u;
        if (!load_float_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        *v = u;
        return true;
    }
    else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1)
    {
        *v = (double)*(const bool *)(cursor->cursor);
        return true;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_string(const flexi_cursor_s *cursor, const char **str)
{
    if (cursor->type != FLEXI_TYPE_STRING)
    {
        return false;
    }

    *str = cursor->cursor;
    return true;
}

/******************************************************************************/

bool flexi_cursor_blob(const flexi_cursor_s *cursor, const uint8_t **blob)
{
    if (cursor->type != FLEXI_TYPE_BLOB)
    {
        return false;
    }

    *blob = (const uint8_t *)cursor->cursor;
    return true;
}

/******************************************************************************/

bool flexi_cursor_typed_vector_data(const flexi_cursor_s *cursor, const void **data)
{
    if (cursor->type == FLEXI_TYPE_VECTOR || !type_is_vector(cursor->type))
    {
        return false;
    }

    *data = cursor->cursor;
    return true;
}

/******************************************************************************/

bool flexi_cursor_vector_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed)
{
    if (cursor->type != FLEXI_TYPE_VECTOR && cursor->type != FLEXI_TYPE_MAP)
    {
        return false;
    }

    return cursor_vector_types(cursor, packed);
}

/******************************************************************************/

bool flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index, flexi_cursor_s *dest)
{
    if (cursor->type != FLEXI_TYPE_VECTOR && cursor->type != FLEXI_TYPE_MAP)
    {
        return false;
    }

    return cursor_seek_vector_index(cursor, index, dest);
}

/******************************************************************************/

bool flexi_cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index, const char **str)
{
    if (cursor->type != FLEXI_TYPE_MAP)
    {
        return false;
    }

    return cursor_map_key_at_index(cursor, index, str);
}

/******************************************************************************/

bool flexi_cursor_seek_map_key(const flexi_cursor_s *cursor, const char *key, flexi_cursor_s *dest)
{
    if (cursor->type != FLEXI_TYPE_MAP)
    {
        return false;
    }

    size_t len = 0;
    if (!cursor_get_len(cursor, &len))
    {
        return false;
    }

    if (len <= 16)
    {
        return cursor_seek_map_key_linear(cursor, key, dest, len);
    }
    else
    {
        return cursor_seek_map_key_bsearch(cursor, key, dest, len);
    }
}

/******************************************************************************/

bool flexi_read(const flexi_reader_s *reader, const flexi_cursor_s *cursor)
{
    switch (cursor->type)
    {
    case FLEXI_TYPE_INT:
    case FLEXI_TYPE_INDIRECT_INT: {
        int64_t u;
        if (!load_sint_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        reader->sint(u);
        return true;
    }
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_UINT: {
        uint64_t u;
        if (!load_uint_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        reader->uint(u);
        return true;
    }
    case FLEXI_TYPE_FLOAT:
    case FLEXI_TYPE_INDIRECT_FLOAT: {
        double u;
        if (!load_float_by_width(&u, cursor->cursor, cursor->stride))
        {
            return false;
        }

        reader->flt(u);
        return true;
    }
    case FLEXI_TYPE_STRING: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->string(cursor->cursor, len);
        return true;
    }
    case FLEXI_TYPE_BLOB: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->blob(cursor->cursor, len);
        return true;
    }
    case FLEXI_TYPE_MAP: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->map_begin(len);
        for (size_t i = 0; i < len; i++)
        {
            const char *key;
            if (!cursor_map_key_at_index(cursor, i, &key))
            {
                return false;
            }

            reader->map_key(key);

            flexi_cursor_s value;
            if (!cursor_seek_vector_index(cursor, i, &value))
            {
                return false;
            }
            if (!flexi_read(reader, &value))
            {
                return false;
            }
        }

        reader->map_end();
        return true;
    }
    case FLEXI_TYPE_VECTOR: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->vector_begin(len);
        for (size_t i = 0; i < len; i++)
        {
            flexi_cursor_s value;
            if (!cursor_seek_vector_index(cursor, i, &value))
            {
                return false;
            }
            if (!flexi_read(reader, &value))
            {
                return false;
            }
        }

        reader->vector_end();
        return true;
    }
    case FLEXI_TYPE_VECTOR_INT: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->typed_vector_sint(cursor->cursor, cursor->stride, len);
        return true;
    }
    case FLEXI_TYPE_VECTOR_UINT: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->typed_vector_uint(cursor->cursor, cursor->stride, len);
        return true;
    }
    case FLEXI_TYPE_VECTOR_FLOAT: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->typed_vector_flt(cursor->cursor, cursor->stride, len);
        return true;
    }
    case FLEXI_TYPE_VECTOR_INT2: {
        reader->typed_vector_sint(cursor->cursor, cursor->stride, 2);
        return true;
    }
    case FLEXI_TYPE_VECTOR_UINT2: {
        reader->typed_vector_uint(cursor->cursor, cursor->stride, 2);
        return true;
    }
    case FLEXI_TYPE_VECTOR_FLOAT2: {
        reader->typed_vector_flt(cursor->cursor, cursor->stride, 2);
        return true;
    }
    case FLEXI_TYPE_VECTOR_INT3: {
        reader->typed_vector_sint(cursor->cursor, cursor->stride, 3);
        return true;
    }
    case FLEXI_TYPE_VECTOR_UINT3: {
        reader->typed_vector_uint(cursor->cursor, cursor->stride, 3);
        return true;
    }
    case FLEXI_TYPE_VECTOR_FLOAT3: {
        reader->typed_vector_flt(cursor->cursor, cursor->stride, 3);
        return true;
    }
    case FLEXI_TYPE_VECTOR_INT4: {
        reader->typed_vector_sint(cursor->cursor, cursor->stride, 4);
        return true;
    }
    case FLEXI_TYPE_VECTOR_UINT4: {
        reader->typed_vector_uint(cursor->cursor, cursor->stride, 4);
        return true;
    }
    case FLEXI_TYPE_VECTOR_FLOAT4: {
        reader->typed_vector_flt(cursor->cursor, cursor->stride, 4);
        return true;
    }
    case FLEXI_TYPE_BOOL: {
        reader->boolean(*(const bool *)(cursor->cursor));
        return true;
    }
    case FLEXI_TYPE_VECTOR_BOOL: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->typed_vector_bool(cursor->cursor, len);
        return true;
    }
    }

    return false;
}
