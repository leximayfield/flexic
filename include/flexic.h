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

#ifndef FLEXI_CONFIG_STACK_LENGTH
/**
 * @brief The size of the stack used by a flexi_writer_s.
 *
 * @details When writing vectors or maps, you push the values you want in
 *          the container onto the stack first.  This stack is hardcoded to
 *          a fixed size, which means that you are limited to this many values
 *          in a single vector or map.
 */
#define FLEXI_CONFIG_STACK_LENGTH (64)
#endif

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Possible error results.  Note that the only guarantee is that
 *        success returns will be > 0 and errors are < 0, exact values of
 *        specific errors are not stable and subject to change.
 */
typedef enum flexi_result_e {
    /**
     * @brief An invalid return value.
     */
    FLEXI_INVALID = 0,

    /**
     * @brief Success.
     */
    FLEXI_OK = 1,

    /**
     * @brief The user passed an invalid parameter.
     */
    FLEXI_ERR_PARAM = -1,

    /**
     * @brief The value at the cursor was out of range for the specified type.
     *        The output value has been populated with the closest value
     *        to the underlying value.
     */
    FLEXI_ERR_RANGE = -2,

    /**
     * @brief Key or index was not found in map or vector.
     */
    FLEXI_ERR_NOTFOUND = -3,

    /**
     * @brief Cursor is not pointing at a valid type for the function that
     *        was called.
     */
    FLEXI_ERR_BADTYPE = -4,

    /**
     * @brief Attempting to read the cursor value or parse the FlexBuffer
     *        resulted in an attempt to read from an invalid location in
     *        said buffer.
     *
     * @details This is usually indicitive of a corrupt or maliciously-
     *          constructed FlexBuffer.
     */
    FLEXI_ERR_BADREAD = -5,

    /**
     * @brief Attempting to parse data resulted in hitting one of the
     *        configured parse limits.
     *
     * @details This is usually indicitive of a corrupt or maliciously-
     *          constructed FlexBuffer, though it could also happen if you
     *          cram one too many vectors or maps in a single FlexBuffer.
     */
    FLEXI_ERR_PARSELIMIT = -6,

    /**
     * @brief A previous write failed, you cannot call another write function
     *        if this happens.
     */
    FLEXI_ERR_FAILSAFE = -7,

    /**
     * @brief While writing data, an invalid stack operation was attempted.
     */
    FLEXI_ERR_BADSTACK = -8,

    /**
     * @brief Attempting to write to the output stream failed.
     */
    FLEXI_ERR_BADWRITE = -9,

    /**
     * @brief A non-write stream operation failed.
     */
    FLEXI_ERR_STREAM = -10,

    /**
     * @brief When creating a map, one of the values in the key array wasn't
     *        actually a key.
     */
    FLEXI_ERR_NOTKEYS = -11,

    /**
     * @brief An internal precondition failed.  End users should never see
     *        this error - if you do, please file a bug.
     */
    FLEXI_ERR_INTERNAL = -12,
} flexi_result_e;

#define FLEXI_SUCCESS(x) (FLEXI_INVALID < (x))
#define FLEXI_ERROR(x) (FLEXI_INVALID > (x))

typedef uint8_t flexi_packed_t;
typedef int flexi_stack_idx_t;

typedef enum flexi_width_e {
    FLEXI_WIDTH_1B,
    FLEXI_WIDTH_2B,
    FLEXI_WIDTH_4B,
    FLEXI_WIDTH_8B,
} flexi_width_e;

typedef enum flexi_type_e {
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

typedef struct flexi_buffer_s {
    const char *data;
    size_t length;
} flexi_buffer_s;

typedef struct flexi_cursor_s {
    flexi_buffer_s buffer;
    const char *cursor;
    flexi_type_e type;
    int width;
} flexi_cursor_s;

typedef struct flexi_parser_s {
    void (*null)(const char *key, void *user);
    void (*sint)(const char *key, int64_t value, void *user);
    void (*uint)(const char *key, uint64_t value, void *user);
    void (*f32)(const char *key, float value, void *user);
    void (*f64)(const char *key, double value, void *user);
    void (*key)(const char *key, const char *str, void *user);
    void (*string)(const char *key, const char *str, size_t len, void *user);
    void (*map_begin)(const char *key, size_t len, void *user);
    void (*map_end)(void *user);
    void (*vector_begin)(const char *key, size_t len, void *user);
    void (*vector_end)(void *user);
    void (*typed_vector)(const char *key, const void *ptr, size_t len,
        flexi_type_e type, int width, void *user);
    void (*blob)(const char *key, const void *ptr, size_t len, void *user);
    void (*boolean)(const char *key, bool val, void *user);
} flexi_parser_s;

typedef struct flexi_value_s {
    union {
        int64_t s64;
        uint64_t u64;
        float f32;
        double f64;
        size_t offset;
    } u;
    const char *key;
    flexi_type_e type;
    int width;
} flexi_value_s;

typedef struct flexi_writer_s {
    flexi_value_s stack[FLEXI_CONFIG_STACK_LENGTH];
    bool (*write_func)(const void *ptr, size_t len, void *user);
    const void *(*data_at_func)(size_t index, void *user);
    bool (*tell_func)(size_t *offset, void *user);
    void *user;
    flexi_stack_idx_t head;
    flexi_result_e err;
} flexi_writer_s;

flexi_buffer_s
flexi_make_buffer(const void *buffer, size_t len);

flexi_result_e
flexi_open_buffer(const flexi_buffer_s *buffer, flexi_cursor_s *cursor);

flexi_type_e
flexi_cursor_type(const flexi_cursor_s *cursor);

int
flexi_cursor_width(const flexi_cursor_s *cursor);

/**
 * @brief Obtain length of any vector-like type: vectors, typed vectors,
 *        strings, and blobs.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @return Length of value at cursor.  Returns 0 on invalid type.
 */
size_t
flexi_cursor_length(const flexi_cursor_s *cursor);

/**
 * @brief Obtain signed int value from cursor.  Types that aren't signed ints
 *        are turned into signed ints on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0 on error.
 * @return FLEXI_OK | FLEXI_ERR_RANGE | FLEXI_ERR_BADTYPE | FLEXI_ERR_BADREAD
 */
flexi_result_e
flexi_cursor_sint(const flexi_cursor_s *cursor, int64_t *val);

/**
 * @brief Obtain unsigned int value from cursor.  Types that aren't unsigned
 *        ints are turned into unsigned ints on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0 on error.
 * @return FLEXI_OK | FLEXI_ERR_RANGE | FLEXI_ERR_BADTYPE | FLEXI_ERR_BADREAD
 */
flexi_result_e
flexi_cursor_uint(const flexi_cursor_s *cursor, uint64_t *val);

/**
 * @brief Obtain 32-bit float value from cursor.  Types that aren't 32-bit
 *        floats are turned into 32-bit floats on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0.0f on error.
 * @return FLEXI_OK | FLEXI_ERR_BADTYPE | FLEXI_ERR_BADREAD
 */
flexi_result_e
flexi_cursor_f32(const flexi_cursor_s *cursor, float *val);

/**
 * @brief Obtain 64-bit float value from cursor.  Types that aren't 64-bit
 *        floats are turned into 64-bit floats on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0.0 on error.
 * @return FLEXI_OK | FLEXI_ERR_BADTYPE | FLEXI_ERR_BADREAD
 */
flexi_result_e
flexi_cursor_f64(const flexi_cursor_s *cursor, double *val);

/**
 * @brief Obtain null-terminated key string from cursor.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] str Obtained string.  Set to "" on error.
 * @return FLEXI_OK | FLEXI_ERR_BADTYPE | FLEXI_ERR_BADREAD
 */
flexi_result_e
flexi_cursor_key(const flexi_cursor_s *cursor, const char **str);

/**
 * @brief Obtain string from cursor.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] str Obtained string.  Set to "" on error.
 * @return FLEXI_OK | FLEXI_ERR_BADTYPE | FLEXI_ERR_BADREAD
 */
flexi_result_e
flexi_cursor_string(const flexi_cursor_s *cursor, const char **str);

/**
 * @brief Given a cursor pointing at a map, return the key located at the
 *        n-th index of the map.
 *
 * @details A map is a vector of keys and a vector of values, so each entry
 *          in the map has an associated index.
 *
 * @param[in] cursor Cursor pointing to map to examine.
 * @param[in] index Index to look up.
 * @param[out] str Key string located at the given index.
 * @return FLEXI_OK | FLEXI_ERR_BADTYPE
 */
flexi_result_e
flexi_cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index,
    const char **str);

/**
 * @brief Given a cursor pointing at a map, create a new cursor pointing
 *        at a value found at the passed key.
 *
 * @details A map is a vector of keys and a vector of values.  Keys are not
 *          hashed, but are sorted in strcmp order, which allows them to
 *          be found via binary search.
 *
 * @param[in] cursor Cursor pointing to map to examine.
 * @param[in] key Key to look up.
 * @param[out] dest Cursor pointing at value for the given key.
 * @return FLEXI_OK | FLEXI_NOTFOUND | FLEXI_BADTYPE | FLEXI_BADREAD
 */
flexi_result_e
flexi_cursor_seek_map_key(const flexi_cursor_s *cursor, const char *key,
    flexi_cursor_s *dest);

flexi_result_e
flexi_cursor_vector_types(const flexi_cursor_s *cursor,
    const flexi_packed_t **packed);

flexi_result_e
flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index,
    flexi_cursor_s *dest);

flexi_result_e
flexi_cursor_typed_vector_data(const flexi_cursor_s *cursor, const void **data);

/**
 * @brief Obtain byte blob from cursor.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] blob Obtained byte blob.  Set to (const uint8_t *)"" on error.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADTYPE Value isn't convertable to blob.
 * @return FLEXI_ERR_BADREAD Out-of-bounds read.
 */
flexi_result_e
flexi_cursor_blob(const flexi_cursor_s *cursor, const uint8_t **blob);

/**
 * @brief Obtain boolean value from cursor.  Non-booleans are turned into
 *        booleans on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to false on error.
 * @return FLEXI_OK on success.
 * @return FLEXI_ERR_BADREAD if read went out of bounds.
 * @return FLEXI_ERR_BADTYPE if type is not convertable to bool.
 */
flexi_result_e
flexi_cursor_bool(const flexi_cursor_s *cursor, bool *val);

/**
 * @brief Starting from the value at the cursor, parse the FlexBuffer while
 *        calling the appropriate callbacks.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] cursor Cursor to start parse at.
 * @param[in] user User pointer - passed to all callbacks.
 * @return FLEXI_OK Successful parse.
 * @return FLEXI_ERR_BADREAD If read went out of range.
 */
flexi_result_e
flexi_parse_cursor(const flexi_parser_s *parser, const flexi_cursor_s *cursor,
    void *user);

flexi_result_e
flexi_write_null_keyed(flexi_writer_s *writer, const char *key);

flexi_result_e
flexi_write_sint_keyed(flexi_writer_s *writer, const char *key, int64_t val);

flexi_result_e
flexi_write_uint_keyed(flexi_writer_s *writer, const char *key, uint64_t v);

flexi_result_e
flexi_write_f32_keyed(flexi_writer_s *writer, const char *key, float val);

flexi_result_e
flexi_write_f64_keyed(flexi_writer_s *writer, const char *key, double val);

flexi_result_e
flexi_write_key_keyed(flexi_writer_s *writer, const char *key, const char *str);

flexi_result_e
flexi_write_string_keyed(flexi_writer_s *writer, const char *key,
    const char *str);

flexi_result_e
flexi_write_indirect_sint_keyed(flexi_writer_s *writer, const char *key,
    int64_t val);

flexi_result_e
flexi_write_indirect_uint_keyed(flexi_writer_s *writer, const char *key,
    uint64_t val);

flexi_result_e
flexi_write_indirect_f32_keyed(flexi_writer_s *writer, const char *key,
    float val);

flexi_result_e
flexi_write_indirect_f64_keyed(flexi_writer_s *writer, const char *key,
    double val);

flexi_result_e
flexi_write_map_keys(flexi_writer_s *writer, size_t len, flexi_width_e stride,
    flexi_stack_idx_t *keyset);

flexi_result_e
flexi_write_map_keyed(flexi_writer_s *writer, const char *key,
    flexi_stack_idx_t keyset, size_t len, flexi_width_e stride);

flexi_result_e
flexi_write_vector_keyed(flexi_writer_s *writer, const char *key, size_t len,
    flexi_width_e stride);

/**
 * @brief Write a typed vector of signed ints to the stream.  Pushes an offset
 *        to the vector onto the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to int array to be written.
 * @param[in] stride Width of each individual int.
 * @param[in] len Number of elements in the array.
 * @return FLEXI_OK | FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_typed_vector_sint_keyed(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, size_t len);

/**
 * @brief Write a typed vector of unsigned ints to the stream.  Pushes an
 *        offset to the vector onto the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to int array to be written.
 * @param[in] stride Width of each individual int.
 * @param[in] len Number of elements in the array.
 * @return FLEXI_OK | FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_typed_vector_uint_keyed(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, size_t len);

/**
 * @brief Write a typed vector of unsigned ints to the stream.  Pushes an
 *        offset to the vector onto the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to int array to be written.
 * @param[in] stride Width of each individual int.
 * @param[in] len Number of elements in the array.
 * @return FLEXI_OK | FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_typed_vector_flt_keyed(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, size_t len);

/**
 * @brief Write a binary blob to the stream.  Pushes an offset to the blob
 *        onto the stack.
 *
 * @details Passing a value for align other than 1 will pad the start of
 *          the blob data to the nearest even multiple of the align value.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to data to be written.
 * @param[in] len Length of data to be written.
 * @param[in] align Desired alignment of blob data, in bytes.
 * @return FLEXI_OK | FLEXI_ERR_BADWRITE | FLEXI_ERR_STREAM.
 */
flexi_result_e
flexi_write_blob_keyed(flexi_writer_s *writer, const char *key, const void *ptr,
    size_t len, int align);

flexi_result_e
flexi_write_bool_keyed(flexi_writer_s *writer, const char *key, bool val);

flexi_result_e
flexi_write_typed_vector_bool_keyed(flexi_writer_s *writer, const char *key,
    const bool *ptr, size_t len);

flexi_result_e
flexi_write_finalize(flexi_writer_s *writer);

static inline flexi_result_e
flexi_write_null(flexi_writer_s *writer)
{
    return flexi_write_null_keyed(writer, NULL);
}

static inline flexi_result_e
flexi_write_sint(flexi_writer_s *writer, int64_t val)
{
    return flexi_write_sint_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_uint(flexi_writer_s *writer, uint64_t val)
{
    return flexi_write_uint_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_f32(flexi_writer_s *writer, float val)
{
    return flexi_write_f32_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_f64(flexi_writer_s *writer, double val)
{
    return flexi_write_f64_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_key(flexi_writer_s *writer, const char *str)
{
    return flexi_write_key_keyed(writer, NULL, str);
}

static inline flexi_result_e
flexi_write_string(flexi_writer_s *writer, const char *str)
{
    return flexi_write_string_keyed(writer, NULL, str);
}

static inline flexi_result_e
flexi_write_indirect_sint(flexi_writer_s *writer, int64_t val)
{
    return flexi_write_indirect_sint_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_indirect_uint(flexi_writer_s *writer, uint64_t val)
{
    return flexi_write_indirect_uint_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_indirect_f32(flexi_writer_s *writer, float val)
{
    return flexi_write_indirect_f32_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_indirect_f64(flexi_writer_s *writer, double val)
{
    return flexi_write_indirect_f64_keyed(writer, NULL, val);
}

static inline flexi_result_e
flexi_write_map(flexi_writer_s *writer, flexi_stack_idx_t keyset, size_t len,
    flexi_width_e stride)
{
    return flexi_write_map_keyed(writer, NULL, keyset, len, stride);
}

static inline flexi_result_e
flexi_write_vector(flexi_writer_s *writer, size_t len, flexi_width_e stride)
{
    return flexi_write_vector_keyed(writer, NULL, len, stride);
}

/**
 * @brief Write a binary blob to the stream.  Pushes an offset to the blob
 *        onto the stack.
 *
 * @details Passing a value for align other than 1 will pad the start of
 *          the blob data to the nearest even multiple of the align value.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] ptr Pointer to data to be written.
 * @param[in] len Length of data to be written.
 * @param[in] align Desired alignment of blob data, in bytes.
 * @return FLEXI_OK | FLEXI_ERR_BADWRITE | FLEXI_ERR_STREAM.
 */
static inline flexi_result_e
flexi_write_blob(flexi_writer_s *writer, const void *ptr, size_t len, int align)
{
    return flexi_write_blob_keyed(writer, NULL, ptr, len, align);
}

static inline flexi_result_e
flexi_write_bool(flexi_writer_s *writer, bool val)
{
    return flexi_write_bool_keyed(writer, NULL, val);
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

template<typename T> static inline flexi_result_e
flexi_write_typed_vector(flexi_writer_s *writer, const T *arr, size_t len);

template<typename T> static inline flexi_result_e
flexi_write_typed_vector_keyed(flexi_writer_s *writer, const char *key,
    const T *arr, size_t len);

template<> inline flexi_result_e
flexi_write_typed_vector<int8_t>(flexi_writer_s *writer, const int8_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_1B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<int8_t>(flexi_writer_s *writer, const char *key,
    const int8_t *arr, size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, key, arr, FLEXI_WIDTH_1B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<int16_t>(flexi_writer_s *writer, const int16_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_2B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<int16_t>(flexi_writer_s *writer, const char *key,
    const int16_t *arr, size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, key, arr, FLEXI_WIDTH_2B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<int32_t>(flexi_writer_s *writer, const int32_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_4B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<int32_t>(flexi_writer_s *writer, const char *key,
    const int32_t *arr, size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, key, arr, FLEXI_WIDTH_4B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<int64_t>(flexi_writer_s *writer, const int64_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_8B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<int64_t>(flexi_writer_s *writer, const char *key,
    const int64_t *arr, size_t len)
{
    return flexi_write_typed_vector_sint_keyed(writer, key, arr, FLEXI_WIDTH_8B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint8_t>(flexi_writer_s *writer, const uint8_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_1B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<uint8_t>(flexi_writer_s *writer, const char *key,
    const uint8_t *arr, size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, key, arr, FLEXI_WIDTH_1B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint16_t>(flexi_writer_s *writer, const uint16_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_2B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<uint16_t>(flexi_writer_s *writer,
    const char *key, const uint16_t *arr, size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, key, arr, FLEXI_WIDTH_2B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint32_t>(flexi_writer_s *writer, const uint32_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_4B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<uint32_t>(flexi_writer_s *writer,
    const char *key, const uint32_t *arr, size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, key, arr, FLEXI_WIDTH_4B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint64_t>(flexi_writer_s *writer, const uint64_t *arr,
    size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, NULL, arr,
        FLEXI_WIDTH_8B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<uint64_t>(flexi_writer_s *writer,
    const char *key, const uint64_t *arr, size_t len)
{
    return flexi_write_typed_vector_uint_keyed(writer, key, arr, FLEXI_WIDTH_8B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<float>(flexi_writer_s *writer, const float *arr,
    size_t len)
{
    return flexi_write_typed_vector_flt_keyed(writer, NULL, arr, FLEXI_WIDTH_4B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<float>(flexi_writer_s *writer, const char *key,
    const float *arr, size_t len)
{
    return flexi_write_typed_vector_flt_keyed(writer, key, arr, FLEXI_WIDTH_4B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<double>(flexi_writer_s *writer, const double *arr,
    size_t len)
{
    return flexi_write_typed_vector_flt_keyed(writer, NULL, arr, FLEXI_WIDTH_8B,
        len);
}

template<> inline flexi_result_e
flexi_write_typed_vector_keyed<double>(flexi_writer_s *writer, const char *key,
    const double *arr, size_t len)
{
    return flexi_write_typed_vector_flt_keyed(writer, key, arr, FLEXI_WIDTH_8B,
        len);
}

#endif
