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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
    typedef uint8_t flexi_packed_t;

    typedef enum _flexi_width_e
    {
        FLEXI_WIDTH_1B,
        FLEXI_WIDTH_2B,
        FLEXI_WIDTH_4B,
        FLEXI_WIDTH_8B,
    } flexi_width_e;

    typedef enum _flexi_type_e
    {
        FLEXI_TYPE_NULL = 0,
        FLEXI_TYPE_INT = 1,
        FLEXI_TYPE_UINT = 2,
        FLEXI_TYPE_FLOAT = 3,
        FLEXI_TYPE_KEY = 4,
        FLEXI_TYPE_STRING = 5,
        FLEXI_TYPE_INDIRECT_INT = 6,
        FLEXI_TYPE_INDIRECT_UINT = 7,
        FLEXI_TYPE_INDIRECT_FLOAT = 8,
        FLEXI_TYPE_MAP = 9,
        FLEXI_TYPE_VECTOR = 10,
        FLEXI_TYPE_VECTOR_INT = 11,
        FLEXI_TYPE_VECTOR_UINT = 12,
        FLEXI_TYPE_VECTOR_FLOAT = 13,
        FLEXI_TYPE_VECTOR_KEY = 14,
        FLEXI_TYPE_VECTOR_STRING = 15,
        FLEXI_TYPE_VECTOR_INT2 = 16,
        FLEXI_TYPE_VECTOR_UINT2 = 17,
        FLEXI_TYPE_VECTOR_FLOAT2 = 18,
        FLEXI_TYPE_VECTOR_INT3 = 19,
        FLEXI_TYPE_VECTOR_UINT3 = 20,
        FLEXI_TYPE_VECTOR_FLOAT3 = 21,
        FLEXI_TYPE_VECTOR_INT4 = 22,
        FLEXI_TYPE_VECTOR_UINT4 = 23,
        FLEXI_TYPE_VECTOR_FLOAT4 = 24,
        FLEXI_TYPE_BLOB = 25,
        FLEXI_TYPE_BOOL = 26,
        FLEXI_TYPE_VECTOR_BOOL = 36,
    } flexi_type_e;

    typedef struct _flexi_buffer_s
    {
        int64_t length;
        const char *data;
    } flexi_buffer_s;

    typedef struct _flexi_cursor_s
    {
        const flexi_buffer_s *buffer;
        const char *cursor;
        flexi_type_e type;
        int stride;
    } flexi_cursor_s;

    flexi_buffer_s flexi_make_buffer(const void *buffer, size_t len);
    bool flexi_buffer_open(const flexi_buffer_s *buffer, flexi_cursor_s *cursor);
    flexi_type_e flexi_cursor_type(const flexi_cursor_s *cursor);
    int flexi_cursor_stride(const flexi_cursor_s *cursor);
    bool flexi_cursor_get_int(const flexi_cursor_s *cursor, int64_t *v);
    bool flexi_cursor_get_uint(const flexi_cursor_s *cursor, uint64_t *v);
    bool flexi_cursor_get_float(const flexi_cursor_s *cursor, float *v);
    bool flexi_cursor_get_double(const flexi_cursor_s *cursor, double *v);
    bool flexi_cursor_get_string(const flexi_cursor_s *cursor, const char **str);
    bool flexi_cursor_get_string_len(const flexi_cursor_s *cursor, size_t *len);
    bool flexi_cursor_get_vector_data(const flexi_cursor_s *cursor, const void **data);
    bool flexi_cursor_get_vector_len(const flexi_cursor_s *cursor, size_t *len);
    bool flexi_cursor_get_vector_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed);
    bool flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index, flexi_cursor_s *dest);
    bool flexi_cursor_get_map_len(const flexi_cursor_s *cursor, size_t *len);
    bool flexi_cursor_get_map_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed);
    bool flexi_cursor_get_map_key(const flexi_cursor_s *cursor, size_t index, const char **str);
    bool flexi_cursor_seek_map_value(const flexi_cursor_s *cursor, size_t index, flexi_cursor_s *dest);

#ifdef __cplusplus
}
#endif
