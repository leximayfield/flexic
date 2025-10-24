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

#define PTR_S8(ptr) ((const int8_t *)(ptr))
#define PTR_S16(ptr) ((const int16_t *)(ptr))
#define PTR_S32(ptr) ((const int32_t *)(ptr))
#define PTR_S64(ptr) ((const int64_t *)(ptr))
#define PTR_U8(ptr) ((const uint8_t *)(ptr))
#define PTR_U16(ptr) ((const uint16_t *)(ptr))
#define PTR_U32(ptr) ((const uint32_t *)(ptr))
#define PTR_U64(ptr) ((const uint64_t *)(ptr))
#define PTR_F32(ptr) ((const float *)(ptr))
#define PTR_F64(ptr) ((const double *)(ptr))
#define PTR_BOOL(ptr) ((const bool *)(ptr))

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
    switch (cursor->stride)
    {
    case 1: *len = *(PTR_U8(cursor->cursor) - 1); return true;
    case 2: *len = *(PTR_U16(cursor->cursor) - 1); return true;
    case 4: *len = *(PTR_U32(cursor->cursor) - 1); return true;
    case 8: *len = *(PTR_U64(cursor->cursor) - 1); return true;
    }
    return false;
}

/******************************************************************************/

static bool cursor_vector_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed)
{
    size_t len = 0;
    if (!cursor_get_len(cursor, &len))
    {
        return false;
    }

    const flexi_packed_t *p = PTR_U8(cursor->cursor);
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

    switch (cursor->stride)
    {
    case 1: offset = *PTR_U8(offset_ptr); break;
    case 2: offset = *PTR_U16(offset_ptr); break;
    case 4: offset = *PTR_U32(offset_ptr); break;
    case 8: offset = *PTR_U64(offset_ptr); break;
    default: return false;
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
    uint64_t keys_offset = 0;
    uint64_t keys_width = 0;
    switch (cursor->stride)
    {
    case 1:
        keys_offset = *(PTR_U8(cursor->cursor) - 3);
        keys_width = *(PTR_U8(cursor->cursor) - 2);
        break;
    case 2:
        keys_offset = *(PTR_U16(cursor->cursor) - 3);
        keys_width = *(PTR_U16(cursor->cursor) - 2);
        break;
    case 4:
        keys_offset = *(PTR_U32(cursor->cursor) - 3);
        keys_width = *(PTR_U32(cursor->cursor) - 2);
        break;
    case 8:
        keys_offset = *(PTR_U64(cursor->cursor) - 3);
        keys_width = *(PTR_U64(cursor->cursor) - 2);
        break;
    default: return false;
    }

    // Seek the keys base.
    const char *keys_base = cursor->cursor - ((3 * cursor->stride) + keys_offset);

    // Resolve the key.
    uint64_t offset = 0;
    const char *offset_ptr = keys_base + (index * (size_t)keys_width);

    switch (keys_width)
    {
    case 1: offset = *PTR_U8(offset_ptr); break;
    case 2: offset = *PTR_U16(offset_ptr); break;
    case 4: offset = *PTR_U32(offset_ptr); break;
    case 8: offset = *PTR_U64(offset_ptr); break;
    default: return false;
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
    uint8_t root_bytes = *PTR_U8(cursor->cursor);
    if (buffer->length < root_bytes + 2)
    {
        return false;
    }

    // Obtain the packed type.
    cursor->cursor -= 1;
    flexi_packed_t packed = *PTR_U8(cursor->cursor);

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
    uint64_t offset = 0;
    switch (root_bytes)
    {
    case 1: offset = *PTR_U8(cursor->cursor); break;
    case 2: offset = *PTR_U16(cursor->cursor); break;
    case 4: offset = *PTR_U32(cursor->cursor); break;
    case 8: offset = *PTR_U64(cursor->cursor); break;
    default: return false;
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

bool flexi_cursor_int(const flexi_cursor_s *cursor, int64_t *v)
{
    if (type_is_anyint(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = *PTR_S8(cursor->cursor); return true;
        case 2: *v = *PTR_S16(cursor->cursor); return true;
        case 4: *v = *PTR_S32(cursor->cursor); return true;
        case 8: *v = *PTR_S64(cursor->cursor); return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        switch (cursor->stride)
        {
        case 4: *v = (int64_t)*PTR_F32(cursor->cursor); return true;
        case 8: *v = (int64_t)*PTR_F64(cursor->cursor); return true;
        }
        return false;
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
        switch (cursor->stride)
        {
        case 1: *v = *PTR_U8(cursor->cursor); return true;
        case 2: *v = *PTR_U16(cursor->cursor); return true;
        case 4: *v = *PTR_U32(cursor->cursor); return true;
        case 8: *v = *PTR_U64(cursor->cursor); return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        switch (cursor->stride)
        {
        case 4: *v = (uint64_t)*PTR_F32(cursor->cursor); return true;
        case 8: *v = (uint64_t)*PTR_F64(cursor->cursor); return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_float(const flexi_cursor_s *cursor, float *v)
{
    if (type_is_int(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (float)*PTR_S8(cursor->cursor); return true;
        case 2: *v = (float)*PTR_S16(cursor->cursor); return true;
        case 4: *v = (float)*PTR_S32(cursor->cursor); return true;
        case 8: *v = (float)*PTR_S64(cursor->cursor); return true;
        }
        return false;
    }
    else if (type_is_uint(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (float)*PTR_U8(cursor->cursor); return true;
        case 2: *v = (float)*PTR_U16(cursor->cursor); return true;
        case 4: *v = (float)*PTR_U32(cursor->cursor); return true;
        case 8: *v = (float)*PTR_U64(cursor->cursor); return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        switch (cursor->stride)
        {
        case 4: *v = *PTR_F32(cursor->cursor); return true;
        case 8: *v = (float)*PTR_F64(cursor->cursor); return true;
        }
        return false;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_double(const flexi_cursor_s *cursor, double *v)
{
    if (type_is_int(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (double)*PTR_S8(cursor->cursor); return true;
        case 2: *v = (double)*PTR_S16(cursor->cursor); return true;
        case 4: *v = (double)*PTR_S32(cursor->cursor); return true;
        case 8: *v = (double)*PTR_S64(cursor->cursor); return true;
        }
        return false;
    }
    else if (type_is_uint(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (double)*PTR_U8(cursor->cursor); return true;
        case 2: *v = (double)*PTR_U16(cursor->cursor); return true;
        case 4: *v = (double)*PTR_U32(cursor->cursor); return true;
        case 8: *v = (double)*PTR_U64(cursor->cursor); return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        switch (cursor->stride)
        {
        case 4: *v = (double)*PTR_F32(cursor->cursor); return true;
        case 8: *v = *PTR_F64(cursor->cursor); return true;
        }
        return false;
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
        switch (cursor->stride)
        {
        case 1: reader->sint(*PTR_S8(cursor->cursor)); return true;
        case 2: reader->sint(*PTR_S16(cursor->cursor)); return true;
        case 4: reader->sint(*PTR_S32(cursor->cursor)); return true;
        case 8: reader->sint(*PTR_S64(cursor->cursor)); return true;
        }
        return false;
    }
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_UINT: {
        switch (cursor->stride)
        {
        case 1: reader->uint(*PTR_U8(cursor->cursor)); return true;
        case 2: reader->uint(*PTR_U16(cursor->cursor)); return true;
        case 4: reader->uint(*PTR_U32(cursor->cursor)); return true;
        case 8: reader->uint(*PTR_U64(cursor->cursor)); return true;
        }
        return false;
    }
    case FLEXI_TYPE_FLOAT:
    case FLEXI_TYPE_INDIRECT_FLOAT: {
        switch (cursor->stride)
        {
        case 4: reader->f32(*PTR_F32(cursor->cursor)); return true;
        case 8: reader->f64(*PTR_F64(cursor->cursor)); return true;
        }
        return false;
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

        switch (cursor->stride)
        {
        case 1: reader->vector_s8(PTR_S8(cursor->cursor), len); return true;
        case 2: reader->vector_s16(PTR_S16(cursor->cursor), len); return true;
        case 4: reader->vector_s32(PTR_S32(cursor->cursor), len); return true;
        case 8: reader->vector_s64(PTR_S64(cursor->cursor), len); return true;
        }
        return false;
    }
    case FLEXI_TYPE_VECTOR_INT2: {
        switch (cursor->stride)
        {
        case 1: reader->vector_s8(PTR_S8(cursor->cursor), 2); return true;
        case 2: reader->vector_s16(PTR_S16(cursor->cursor), 2); return true;
        case 4: reader->vector_s32(PTR_S32(cursor->cursor), 2); return true;
        case 8: reader->vector_s64(PTR_S64(cursor->cursor), 2); return true;
        }
        return false;
    }
    case FLEXI_TYPE_VECTOR_INT3: {
        switch (cursor->stride)
        {
        case 1: reader->vector_s8(PTR_S8(cursor->cursor), 3); return true;
        case 2: reader->vector_s16(PTR_S16(cursor->cursor), 3); return true;
        case 4: reader->vector_s32(PTR_S32(cursor->cursor), 3); return true;
        case 8: reader->vector_s64(PTR_S64(cursor->cursor), 3); return true;
        }
        return false;
    }
    case FLEXI_TYPE_VECTOR_INT4: {
        switch (cursor->stride)
        {
        case 1: reader->vector_s8(PTR_S8(cursor->cursor), 4); return true;
        case 2: reader->vector_s16(PTR_S16(cursor->cursor), 4); return true;
        case 4: reader->vector_s32(PTR_S32(cursor->cursor), 4); return true;
        case 8: reader->vector_s64(PTR_S64(cursor->cursor), 4); return true;
        }
        return false;
    }
    case FLEXI_TYPE_BOOL: {
        reader->boolean(*PTR_BOOL(cursor->cursor));
        return true;
    }
    case FLEXI_TYPE_VECTOR_BOOL: {
        size_t len;
        if (!cursor_get_len(cursor, &len))
        {
            return false;
        }

        reader->vector_boolean(PTR_BOOL(cursor->cursor), len);
        return true;
    }
    }

    return false;
}
