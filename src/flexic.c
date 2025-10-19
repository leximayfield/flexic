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

/******************************************************************************/

static flexi_type_e unpack_type(flexi_packed_t packed)
{
    return packed >> 2;
}

/******************************************************************************/

static flexi_width_e unpack_width(flexi_packed_t packed)
{
    return packed & 0x03;
}

/******************************************************************************/

static int width_to_bytes(flexi_width_e width)
{
    return 1 << width;
}

/******************************************************************************/

static bool bytes_to_width(flexi_width_e *width, int bytes)
{
    switch (bytes)
    {
    case 1: *width = FLEXI_WIDTH_1B; return true;
    case 2: *width = FLEXI_WIDTH_2B; return true;
    case 4: *width = FLEXI_WIDTH_4B; return true;
    case 8: *width = FLEXI_WIDTH_8B; return true;
    }
    return false;
}

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

static bool cursor_resolve_offset(const uint8_t **dest, const flexi_buffer_s *buffer, const uint8_t *src,
                                  uint64_t offset)
{
    if (offset > PTRDIFF_MAX)
    {
        return false;
    }

    ptrdiff_t move = (ptrdiff_t)offset;
    ptrdiff_t max_move = (const char *)src - buffer->data;
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
    case 1: *len = *(cursor->cursor.pb - 1); return true;
    case 2: *len = *(cursor->cursor.pw - 1); return true;
    case 4: *len = *(cursor->cursor.pdw - 1); return true;
    case 8: *len = *(cursor->cursor.pqw - 1); return true;
    }
    return false;
}

/******************************************************************************/

static bool cursor_get_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed)
{
    size_t len = 0;
    if (!cursor_get_len(cursor, &len))
    {
        return false;
    }

    const flexi_packed_t *p = cursor->cursor.pb;
    *packed = p + len * cursor->stride;
    return true;
}

/******************************************************************************/

flexi_buffer_s flexi_make_buffer(const void *buffer, size_t len)
{
    flexi_buffer_s rvo;
    rvo.data = buffer;
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
    cursor->cursor.pb = (const uint8_t *)(buffer->data) + buffer->length - 1;
    uint8_t root_bytes = *cursor->cursor.pb;
    if (buffer->length < root_bytes + 2)
    {
        return false;
    }

    // Obtain the packed type.
    cursor->cursor.pb -= 1;
    flexi_packed_t packed = *cursor->cursor.pb;

    // Point at the root object.
    flexi_type_e type = unpack_type(packed);
    cursor->cursor.pb -= root_bytes;
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
    case 1: offset = *cursor->cursor.pb; break;
    case 2: offset = *cursor->cursor.pw; break;
    case 4: offset = *cursor->cursor.pdw; break;
    case 8: offset = *cursor->cursor.pqw; break;
    default: return false;
    }

    uint8_t *dest = NULL;
    if (!cursor_resolve_offset(&dest, cursor->buffer, cursor->cursor.pb, offset))
    {
        return false;
    }

    cursor->cursor.pb = dest;
    cursor->type = type;
    cursor->stride = width_to_bytes(unpack_width(packed));
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

bool flexi_cursor_get_int(const flexi_cursor_s *cursor, int64_t *v)
{
    if (type_is_anyint(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (int64_t)(*(int8_t *)cursor->cursor.pb); return true;
        case 2: *v = (int64_t)(*(int16_t *)cursor->cursor.pw); return true;
        case 4: *v = (int64_t)(*(int32_t *)cursor->cursor.pdw); return true;
        case 8: *v = (int64_t)(*(int64_t *)cursor->cursor.pqw); return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        if (cursor->stride == 4)
        {
            *v = (int64_t)(*(float *)cursor->cursor.pdw);
            return true;
        }
        else if (cursor->stride == 8)
        {
            *v = (int64_t)(*(double *)cursor->cursor.pqw);
            return true;
        }

        return false;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_get_uint(const flexi_cursor_s *cursor, uint64_t *v)
{
    if (type_is_anyint(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = *cursor->cursor.pb; return true;
        case 2: *v = *cursor->cursor.pw; return true;
        case 4: *v = *cursor->cursor.pdw; return true;
        case 8: *v = *cursor->cursor.pqw; return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        if (cursor->stride == 4)
        {
            *v = (uint64_t)(*(float *)cursor->cursor.pdw);
            return true;
        }
        else if (cursor->stride == 8)
        {
            *v = (uint64_t)(*(double *)cursor->cursor.pqw);
            return true;
        }

        return false;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_get_float(const flexi_cursor_s *cursor, float *v)
{
    if (type_is_int(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (float)(*(int8_t *)cursor->cursor.pb); return true;
        case 2: *v = (float)(*(int16_t *)cursor->cursor.pw); return true;
        case 4: *v = (float)(*(int32_t *)cursor->cursor.pdw); return true;
        case 8: *v = (float)(*(int64_t *)cursor->cursor.pqw); return true;
        }
        return false;
    }
    else if (type_is_uint(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (float)*cursor->cursor.pb; return true;
        case 2: *v = (float)*cursor->cursor.pw; return true;
        case 4: *v = (float)*cursor->cursor.pdw; return true;
        case 8: *v = (float)*cursor->cursor.pqw; return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        if (cursor->stride == 4)
        {
            *v = *(float *)cursor->cursor.pdw;
            return true;
        }
        else if (cursor->stride == 8)
        {
            *v = (float)(*(double *)cursor->cursor.pqw);
            return true;
        }

        return false;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_get_double(const flexi_cursor_s *cursor, double *v)
{
    if (type_is_int(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (double)(*(int8_t *)cursor->cursor.pb); return true;
        case 2: *v = (double)(*(int16_t *)cursor->cursor.pw); return true;
        case 4: *v = (double)(*(int32_t *)cursor->cursor.pdw); return true;
        case 8: *v = (double)(*(int64_t *)cursor->cursor.pqw); return true;
        }
        return false;
    }
    else if (type_is_uint(cursor->type))
    {
        switch (cursor->stride)
        {
        case 1: *v = (double)*cursor->cursor.pb; return true;
        case 2: *v = (double)*cursor->cursor.pw; return true;
        case 4: *v = (double)*cursor->cursor.pdw; return true;
        case 8: *v = (double)*cursor->cursor.pqw; return true;
        }
        return false;
    }
    else if (type_is_float(cursor->type))
    {
        if (cursor->stride == 4)
        {
            *v = (double)(*(float *)cursor->cursor.pdw);
            return true;
        }
        else if (cursor->stride == 8)
        {
            *v = *(double *)cursor->cursor.pqw;
            return true;
        }

        return false;
    }
    else
    {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_get_string(const flexi_cursor_s *cursor, const char **str)
{
    if (cursor->type != FLEXI_TYPE_STRING)
    {
        return false;
    }

    *str = (const char *)cursor->cursor.pb;
    return true;
}

/******************************************************************************/

bool flexi_cursor_get_string_len(const flexi_cursor_s *cursor, size_t *len)
{
    if (cursor->type != FLEXI_TYPE_STRING)
    {
        return false;
    }

    return cursor_get_len(cursor, len);
}

/******************************************************************************/

bool flexi_cursor_get_vector_len(const flexi_cursor_s *cursor, size_t *len)
{
    if (!type_is_vector(cursor->type))
    {
        return false;
    }

    return cursor_get_len(cursor, len);
}

/******************************************************************************/

bool flexi_cursor_get_vector_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed)
{
    if (cursor->type != FLEXI_TYPE_VECTOR)
    {
        return false;
    }

    return cursor_get_types(cursor, packed);
}

/******************************************************************************/

bool flexi_cursor_get_map_len(const flexi_cursor_s *cursor, size_t *len)
{
    if (cursor->type != FLEXI_TYPE_MAP)
    {
        return false;
    }

    return cursor_get_len(cursor, len);
}

/******************************************************************************/

bool flexi_cursor_get_map_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed)
{
    if (cursor->type != FLEXI_TYPE_MAP)
    {
        return false;
    }

    return cursor_get_types(cursor, packed);
}
