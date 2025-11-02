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
extern "C" {
#endif
typedef uint8_t flexi_packed_t;

typedef enum {
    FLEXI_WIDTH_1B,
    FLEXI_WIDTH_2B,
    FLEXI_WIDTH_4B,
    FLEXI_WIDTH_8B,
} flexi_width_e;

typedef enum {
    FLEXI_TYPE_NULL = 0,
    FLEXI_TYPE_SINT = 1,
    FLEXI_TYPE_UINT = 2,
    FLEXI_TYPE_FLOAT = 3,
    FLEXI_TYPE_KEY = 4,
    FLEXI_TYPE_STRING = 5,
    FLEXI_TYPE_INDIRECT_SINT = 6,
    FLEXI_TYPE_INDIRECT_UINT = 7,
    FLEXI_TYPE_INDIRECT_FLOAT = 8,
    FLEXI_TYPE_MAP = 9,
    FLEXI_TYPE_VECTOR = 10,
    FLEXI_TYPE_VECTOR_SINT = 11,
    FLEXI_TYPE_VECTOR_UINT = 12,
    FLEXI_TYPE_VECTOR_FLOAT = 13,
    FLEXI_TYPE_VECTOR_KEY = 14,
    FLEXI_TYPE_VECTOR_SINT2 = 16,
    FLEXI_TYPE_VECTOR_UINT2 = 17,
    FLEXI_TYPE_VECTOR_FLOAT2 = 18,
    FLEXI_TYPE_VECTOR_SINT3 = 19,
    FLEXI_TYPE_VECTOR_UINT3 = 20,
    FLEXI_TYPE_VECTOR_FLOAT3 = 21,
    FLEXI_TYPE_VECTOR_SINT4 = 22,
    FLEXI_TYPE_VECTOR_UINT4 = 23,
    FLEXI_TYPE_VECTOR_FLOAT4 = 24,
    FLEXI_TYPE_BLOB = 25,
    FLEXI_TYPE_BOOL = 26,
    FLEXI_TYPE_VECTOR_BOOL = 36,
} flexi_type_e;

typedef struct {
    int64_t length;
    const char *data;
} flexi_buffer_s;

typedef struct {
    const flexi_buffer_s *buffer;
    const char *cursor;
    flexi_type_e type;
    int stride;
} flexi_cursor_s;

typedef struct {
    void (*null)(void);
    void (*sint)(int64_t);
    void (*uint)(uint64_t);
    void (*f32)(float);
    void (*f64)(double);
    void (*key)(const char *str);
    void (*string)(const char *str, size_t len);
    void (*blob)(const void *ptr, size_t len);
    void (*map_begin)(size_t len);
    void (*map_key)(const char *str);
    void (*map_end)(void);
    void (*vector_begin)(size_t len);
    void (*vector_end)(void);
    void (*typed_vector_sint)(const void *ptr, int width, size_t len);
    void (*typed_vector_uint)(const void *ptr, int width, size_t len);
    void (*typed_vector_flt)(const void *ptr, int width, size_t len);
    void (*boolean)(bool value);
    void (*typed_vector_bool)(const bool *ptr, size_t len);
} flexi_reader_s;

typedef struct {
    union {
        int64_t s64;
        uint64_t u64;
        float f32;
        double f64;
        size_t offset;
    } u;
    flexi_type_e type;
    int width;
} flexi_value_s;

typedef struct {
    flexi_value_s stack[256];
    bool (*write_func)(const void *ptr, size_t len, void *user);
    const void *(*data_at_func)(size_t index, void *user);
    bool (*tell_func)(size_t *offset, void *user);
    void *user;
    int head;
} flexi_writer_s;

flexi_buffer_s flexi_make_buffer(const void *buffer, size_t len);
bool flexi_buffer_open(const flexi_buffer_s *buffer, flexi_cursor_s *cursor);
flexi_type_e flexi_cursor_type(const flexi_cursor_s *cursor);
int flexi_cursor_stride(const flexi_cursor_s *cursor);
bool flexi_cursor_length(const flexi_cursor_s *cursor, size_t *len);
bool flexi_cursor_bool(const flexi_cursor_s *cursor, bool *v);
bool flexi_cursor_sint(const flexi_cursor_s *cursor, int64_t *v);
bool flexi_cursor_uint(const flexi_cursor_s *cursor, uint64_t *v);
bool flexi_cursor_f32(const flexi_cursor_s *cursor, float *v);
bool flexi_cursor_f64(const flexi_cursor_s *cursor, double *v);
bool flexi_cursor_string(const flexi_cursor_s *cursor, const char **str);
bool flexi_cursor_key(const flexi_cursor_s *cursor, const char **str);
bool flexi_cursor_blob(const flexi_cursor_s *cursor, const uint8_t **blob);
bool flexi_cursor_typed_vector_data(const flexi_cursor_s *cursor,
                                    const void **data);
bool flexi_cursor_vector_types(const flexi_cursor_s *cursor,
                               const flexi_packed_t **packed);
bool flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index,
                                    flexi_cursor_s *dest);
bool flexi_cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index,
                                   const char **str);
bool flexi_cursor_seek_map_key(const flexi_cursor_s *cursor, const char *key,
                               flexi_cursor_s *dest);

bool flexi_read(const flexi_reader_s *reader, const flexi_cursor_s *cursor);

bool flexi_write_bool(flexi_writer_s *writer, bool v);
bool flexi_write_sint(flexi_writer_s *writer, int64_t v);
bool flexi_write_uint(flexi_writer_s *writer, uint64_t v);
bool flexi_write_f32(flexi_writer_s *writer, float v);
bool flexi_write_f64(flexi_writer_s *writer, double v);
bool flexi_write_string(flexi_writer_s *writer, const char *str);
bool flexi_write_key(flexi_writer_s *writer, const char *str);
bool flexi_write_blob(flexi_writer_s *writer, const void *ptr, size_t len);
bool flexi_write_indirect_sint(flexi_writer_s *writer, int64_t v);
bool flexi_write_indirect_uint(flexi_writer_s *writer, uint64_t v);
bool flexi_write_indirect_f32(flexi_writer_s *writer, float v);
bool flexi_write_indirect_f64(flexi_writer_s *writer, double v);
bool flexi_write_inline_map(flexi_writer_s *writer, size_t len,
                            flexi_width_e stride);
bool flexi_write_vector(flexi_writer_s *writer, size_t len,
                        flexi_width_e stride);
bool flexi_write_finalize(flexi_writer_s *writer);

#ifdef __cplusplus
}
#endif
