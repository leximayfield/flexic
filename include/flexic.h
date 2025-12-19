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

#if defined(__cplusplus)
#if defined(_MSVC_LANG)
#define FLEXI_CPLUSPLUS _MSVC_LANG
#else
#define FLEXI_CPLUSPLUS __cplusplus
#endif
#else
#define FLEXI_CPLUSPLUS 0L
#endif

#if FLEXI_CPLUSPLUS >= 201103L
#define FLEXI_CPP11_EQ_DELETE = delete
#else
#define FLEXI_CPP11_EQ_DELETE
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if FLEXI_CPLUSPLUS
extern "C" {
#endif

/**
 * @brief The base size type is signed.  This helps avoid wraparound bugs.
 */
typedef ptrdiff_t flexi_ssize_t;
#define FLEXI_SSIZE_MIN ((ptrdiff_t)-1)
#define FLEXI_SSIZE_MAX (PTRDIFF_MAX)

/**
 * @brief A stack index is an opaque type to a position on the stack.
 *        Currently only used when creating maps, which requires you to
 *        stamp out a vector of keys on the stack for future reference.
 */
typedef flexi_ssize_t flexi_stack_idx_t;

/**
 * @brief Possible error results.
 *
 * @details Note that the only guarantee is that success returns will be > 0
 *          and errors are < 0, exact values of specific errors are not stable
 *          and subject to change.  0 is an invalid result to prevent careless
 *          shortcuts that check truthiness instead of using FLEXI_SUCCESS.
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
     * @brief For cursor operations, the cursor you attmpted to pass isn't
     *        valid due to the failure of an earlier cursor operation.  For
     *        write operations, the writer you attempted to pass is in an
     *        invalid state due to an unrecoverable write error.
     */
    FLEXI_ERR_FAILSAFE = -7,

    /**
     * @brief An invalid writing stack operation was attempted.
     */
    FLEXI_ERR_BADSTACK = -8,

    /**
     * @brief An operation done on the output stream, usually a write, failed.
     */
    FLEXI_ERR_BADWRITE = -9,

    /**
     * @brief When creating a map, one of the values in the key array wasn't
     *        actually a key.
     */
    FLEXI_ERR_NOTKEYS = -10,

    /**
     * @brief An internal precondition failed.  End users should never see
     *        this error - if you do, please file a bug.
     */
    FLEXI_ERR_INTERNAL = -11,
} flexi_result_e;

/**
 * @brief Macro which checks for any success value.
 */
#define FLEXI_SUCCESS(x) (FLEXI_INVALID < (x))

/**
 * @brief Macro which checks for any error.
 */
#define FLEXI_ERROR(x) (FLEXI_INVALID > (x))

/**
 * @brief Possible values for the low 2 bits of a packed type.
 */
typedef enum flexi_width_e {
    FLEXI_WIDTH_1B = 0,
    FLEXI_WIDTH_2B = 1,
    FLEXI_WIDTH_4B = 2,
    FLEXI_WIDTH_8B = 3,
} flexi_width_e;

/**
 * @brief Possible values for the high 6 bits of a packed type.
 *
 * @note There are two types of values, direct and indirect.
 *
 *       When writing out vectors (and maps, which are similarly shaped),
 *       direct values are placed directly in the vector, while indirect
 *       values are stored at some point before the vector and are pointed
 *       to from inside the vector with an offset value.
 *
 *       Thus, direct values are cache-friendly but waste space, while
 *       indirect values aren't as cache-friendly but can contain data that
 *       is much larger.
 */
typedef enum flexi_type_e {
    /**
     * @brief A null value of 0.
     */
    FLEXI_TYPE_NULL = 0,
    /**
     * @brief A signed integer which is stored directly.
     */
    FLEXI_TYPE_SINT = 1,
    /**
     * @brief An unsigned integer which is stored directly inside any vector
     *        or map.
     */
    FLEXI_TYPE_UINT = 2,
    /**
     * @brief A float which is stored directly inside any vector or map.
     */
    FLEXI_TYPE_FLOAT = 3,
    /**
     * @brief A null-terminated string.  Stored indirectly.
     */
    FLEXI_TYPE_KEY = 4,
    /**
     * @brief A string which knows its own length.  This is assumed to be
     *        in UTF-8 format.  Stored indirectly.
     */
    FLEXI_TYPE_STRING = 5,
    /**
     * @brief A signed integer which is stored indirectly.
     */
    FLEXI_TYPE_INDIRECT_SINT = 6,
    /**
     * @brief An unsigned integer which is stored indirectly.
     */
    FLEXI_TYPE_INDIRECT_UINT = 7,
    /**
     * @brief A float which is stored indirectly.
     */
    FLEXI_TYPE_INDIRECT_FLOAT = 8,
    /**
     * @brief A map type - think dictionary or hashtable.
     *
     * @details Maps are essentially two vectors, one containing keys and
     *          one containing values.  The keys of a map are sorted in strcmp
     *          order, which allows them to be found via binary search.
     *          Values are sorted with the same order as the keys.  This
     *          is an implementation detail - you as the user do not need
     *          to sort the keys yourself.
     *
     *          Map values have the following wire format.  Length, values,
     *          and key vector metadata are all stored according to the
     *          configured stride.  Types are always tightly packed to a
     *          single byte.
     *
     *          [-3] Offset to the keys vector.
     *          [-2] Byte width of keys vector.
     *          [-1] Length.
     *          [ 0] Map value/offset 0.
     *          [ 1] Map value/offset 1.
     *          ...
     *          [ n] Map value/offset n.
     *          [t0] Type of map value 0.
     *          [t1] Type of map value 1.
     *          ...
     *          [t1] Type of map value n.
     */
    FLEXI_TYPE_MAP = 9,
    /**
     * @brief A vector which can contain values of any type, direct or
     *        indirect.
     *
     * @details Vectors have the following wire format.  Length, values, and
     *          offsets are stored according to the configured stride.  Types
     *          are always tightly packed to a single byte.
     *
     *          [-1] Length.
     *          [ 0] Vector value/offset 0.
     *          [ 1] Vector value/offset 1.
     *          ...
     *          [ n] Vector value/offset n.
     *          [t0] Type of vector value 0.
     *          [t1] Type of vector value 1.
     *          ...
     *          [t1] Type of vector value n.
     */
    FLEXI_TYPE_VECTOR = 10,
    /**
     * @brief A typed vector which only contains direct signed ints.
     *
     * @details Typed vectors do not have trailing types.
     */
    FLEXI_TYPE_VECTOR_SINT = 11,
    /**
     * @brief A typed vector which only contains direct unsigned ints.
     */
    FLEXI_TYPE_VECTOR_UINT = 12,
    /**
     * @brief A typed vector which only contains direct floats.
     */
    FLEXI_TYPE_VECTOR_FLOAT = 13,
    /**
     * @brief A typed vector which only contains offsets to keys.
     */
    FLEXI_TYPE_VECTOR_KEY = 14,
    /**
     * @brief A typed vector which contains exactly 2 direct signed ints.
     *
     * @notes Exact-length typed vectors do not have a length stored at
     *        offset -1.
     */
    FLEXI_TYPE_VECTOR_SINT2 = 16,
    /**
     * @brief A typed vector which contains exactly 2 direct unsigned ints.
     */
    FLEXI_TYPE_VECTOR_UINT2 = 17,
    /**
     * @brief A typed vector which contains exactly 2 direct floats.
     */
    FLEXI_TYPE_VECTOR_FLOAT2 = 18,
    /**
     * @brief A typed vector which contains exactly 3 direct signed ints.
     */
    FLEXI_TYPE_VECTOR_SINT3 = 19,
    /**
     * @brief A typed vector which contains exactly 3 direct unsigned ints.
     */
    FLEXI_TYPE_VECTOR_UINT3 = 20,
    /**
     * @brief A typed vector which contains exactly 3 direct floats.
     */
    FLEXI_TYPE_VECTOR_FLOAT3 = 21,
    /**
     * @brief A typed vector which contains exactly 4 direct signed ints.
     */
    FLEXI_TYPE_VECTOR_SINT4 = 22,
    /**
     * @brief A typed vector which contains exactly 4 direct unsigned ints.
     */
    FLEXI_TYPE_VECTOR_UINT4 = 23,
    /**
     * @brief A typed vector which contains exactly 4 direct floats.
     */
    FLEXI_TYPE_VECTOR_FLOAT4 = 24,
    /**
     * @brief A binary buffer, stored indirectly.
     */
    FLEXI_TYPE_BLOB = 25,
    /**
     * @brief A boolean value, stored directly.
     */
    FLEXI_TYPE_BOOL = 26,
    /**
     * @brief A typed vector of directly-stored boolean values.
     */
    FLEXI_TYPE_VECTOR_BOOL = 36,
} flexi_type_e;

/**
 * @brief A packed FlexBuffer type packs the width into the lower 2 bits
 *        and the wire type into the high 6 bits.
 */
typedef uint8_t flexi_packed_t;

/**
 * @brief Extract underlying type enum from packed type.
 */
#define FLEXI_UNPACK_TYPE(packed) ((flexi_type_e)(packed >> 2))

/**
 * @brief Extract underlying width enum from packed type.
 */
#define FLEXI_UNPACK_WIDTH(packed) ((flexi_width_e)(packed & 0x03))

/**
 * @brief Convert width enum to byte count integer.
 */
#define FLEXI_WIDTH_TO_BYTES(width) ((int)(1 << width))

/**
 * @brief Error type sentinal value, used by flexi_cursor_s.
 */
extern const flexi_type_e FLEXI_TYPE_INVALID;

typedef struct flexi_buffer_s {
    const char *data;
    flexi_ssize_t length;
} flexi_buffer_s;

typedef struct flexi_cursor_s {
    flexi_buffer_s buffer;
    const char *cursor;
    flexi_type_e type;
    int width;
    flexi_ssize_t length;
} flexi_cursor_s;

/**
 * @brief Function called on every iteration of flexi_cursor_foreach.
 *
 * @param key Key assigned to a particular map value, or NULL if iterable
 *            is not a map.
 * @param value Value currently being iterated over.
 * @param user User pointer passed to flexi_cursor_foreach.
 */
typedef bool (*flexi_foreach_fn)(const char *key, flexi_cursor_s *value,
    void *user);

/**
 * @brief Create a buffer from a void pointer/len.
 */
flexi_buffer_s
flexi_make_buffer(const void *buffer, flexi_ssize_t len);

/**
 * @brief "Open" a buffer and seek to the root object.
 *
 * @param[in] buffer Buffer to open.
 * @param[out] cursor Cursor pointing to root object.
 * @return FLEXI_OK || FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_open_buffer(const flexi_buffer_s *buffer, flexi_cursor_s *cursor);

/**
 * @brief Obtain the type of the value pointed to by the cursor.
 *
 * @param[in] cursor Cursor to examine.
 * @return Enumerated type of cursor, or FLEXI_TYPE_INVALID on error.
 */
flexi_type_e
flexi_cursor_type(const flexi_cursor_s *cursor);

/**
 * @brief Obtain the width or stride of the value pointed to by the cursor.
 *
 * @param[in] cursor Cursor to examine.
 * @return Width of cursor in bytes, or 0 on error.
 */
int
flexi_cursor_width(const flexi_cursor_s *cursor);

/**
 * @brief Obtain length of any vector-like type: vectors, typed vectors,
 *        strings, and blobs.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @return Length of value at cursor.  Returns 0 on invalid type.
 */
flexi_ssize_t
flexi_cursor_length(const flexi_cursor_s *cursor);

/**
 * @brief Obtain signed int value from cursor.  Types that aren't signed ints
 *        are turned into signed ints on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0 on error.
 * @return FLEXI_OK || FLEXI_ERR_RANGE || FLEXI_ERR_BADTYPE ||
 *         FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_cursor_sint(const flexi_cursor_s *cursor, int64_t *val);

/**
 * @brief Obtain unsigned int value from cursor.  Types that aren't unsigned
 *        ints are turned into unsigned ints on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0 on error.
 * @return FLEXI_OK || FLEXI_ERR_RANGE || FLEXI_ERR_BADTYPE ||
 *         FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_cursor_uint(const flexi_cursor_s *cursor, uint64_t *val);

/**
 * @brief Obtain 32-bit float value from cursor.  Types that aren't 32-bit
 *        floats are turned into 32-bit floats on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0.0f on error.
 * @return FLEXI_OK || FLEXI_ERR_BADTYPE || FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_cursor_f32(const flexi_cursor_s *cursor, float *val);

/**
 * @brief Obtain 64-bit float value from cursor.  Types that aren't 64-bit
 *        floats are turned into 64-bit floats on a best-effort basis.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] val Obtained value.  Set to 0.0 on error.
 * @return FLEXI_OK || FLEXI_ERR_BADTYPE || FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_cursor_f64(const flexi_cursor_s *cursor, double *val);

/**
 * @brief Obtain null-terminated key string from cursor.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] str Obtained string.  Set to "" on error.
 * @return FLEXI_OK || FLEXI_ERR_BADTYPE || FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_cursor_key(const flexi_cursor_s *cursor, const char **str);

/**
 * @brief Obtain string from cursor.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] str Obtained string.  Set to "" on error.
 * @param[out] len Length of string.  Set to 0 on error.
 * @return FLEXI_OK || FLEXI_ERR_BADTYPE || FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_cursor_string(const flexi_cursor_s *cursor, const char **str,
    flexi_ssize_t *len);

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
 * @return FLEXI_OK || FLEXI_ERR_BADTYPE.
 */
flexi_result_e
flexi_cursor_map_key_at_index(const flexi_cursor_s *cursor, flexi_ssize_t index,
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
 * @param[out] dest Cursor pointing at value for the given key, or pointer
 *                  to failsafe cursor.
 * @return FLEXI_OK || FLEXI_NOTFOUND || FLEXI_BADTYPE || FLEXI_BADREAD
 */
flexi_result_e
flexi_cursor_seek_map_key(const flexi_cursor_s *cursor, const char *key,
    flexi_cursor_s *dest);

/**
 * @brief Given a cursor pointing at an untyped vector or map, return a pointer
 *        to the packed type value for index 0.  The pointer is to a tightly
 *        packed array of data, so the type value for index n is `packed[n]`
 *        or `packed + n`.
 *
 * @param[in] cursor Cursor pointing to map or vector to examine.
 * @param[out] packed Pointer to packed type, or pointer to hardcoded failsafe
 *             value of FLEXI_TYPE_NULL.
 * @return FLEXI_OK || FLEXI_ERR_FAILSAFE || FLEXI_ERR_BADTYPE.
 */
flexi_result_e
flexi_cursor_vector_types(const flexi_cursor_s *cursor,
    const flexi_packed_t **packed);

/**
 * @brief Given a cursor pointing to a vector or map, return a second cursor
 *        pointing to the value at indexed position.
 *
 * @note Maps are also vectors which are sorted by key order, which are in
 *       turn sorted in strcmp order.  You can access them like any other
 *       vector if you know the order of the keys.
 *
 * @param[in] cursor Cursor pointing to map or vector to seek into.
 * @param[in] index Index to look up.
 * @param[out] dest Cursor pointing at value for the given index, or pointer
 *                  to failsafe cursor on error.
 * @return FLEXI_OK || FLEXI_ERR_BADTYPE || FLEXI_ERR_FAILSAFE.
 */
flexi_result_e
flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor,
    flexi_ssize_t index, flexi_cursor_s *dest);

/**
 * @brief Given a cursor pointing at a typed vector, obtain a pointer to
 *        its initial value, as well as the type and stride of the vector.
 *
 * @note The return value is void* because the library cannot assume that
 *       the underlying buffer is properly aligned.  This could cause unaligned
 *       access on platforms where unaligned access is slow or even completely
 *       invalid.
 *
 *       If you're on a platform where unaligned access is fine, feel free
 *       to cast the pointer.  If you're on a platform that doesn't appreciate
 *       that sort of thing, memcpy the data into an aligned array.
 *
 * @param[in] cursor Cursor pointing to typed vector.
 * @param[out] data Pointer to start of data, or pointer a single uint64_t
 *                  on failure.
 * @param[in] type Type of vector, or FLEXI_TYPE_INVALID on error.
 * @param[in] stride Stride of vector values, or 0 on error.
 * @param[in] count Number of items in the vector, or 0 on error.
 * @return FLEXI_OK || FLEXI_FAILSAFE || FLEXI_BADTYPE.
 */
flexi_result_e
flexi_cursor_typed_vector_data(const flexi_cursor_s *cursor, const void **data,
    flexi_type_e *type, int *stride, flexi_ssize_t *count);

/**
 * @brief Iterate over a map or vector type.
 *
 * @param[in] cursor Cursor pointing to map or vector.
 * @param[in] foreach Function which will be called per iteration.
 * @param[in] user User pointer which will be passed to foreach function.
 * @return FLEXI_OK || FLEXI_BADREAD.
 */
flexi_result_e
flexi_cursor_foreach(flexi_cursor_s *cursor, flexi_foreach_fn foreach,
    void *user);

/**
 * @brief Obtain byte blob from cursor.
 *
 * @param[in] cursor Cursor pointing to value to examine.
 * @param[out] blob Obtained byte blob.  Set to 8 bit stub buffer on error.
 * @param[out] len Length of obtained byte blob.  Set to 0 on error.
 * @return FLEXI_OK || FLEXI_ERR_BADTYPE || FLEXI_ERR_BADREAD.
 */
flexi_result_e
flexi_cursor_blob(const flexi_cursor_s *cursor, const uint8_t **blob,
    flexi_ssize_t *len);

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

/******************************************************************************/

/**
 * @brief A collection of callbacks which will be called when a parse function
 *        is called.
 */
typedef struct flexi_parser_s {
    void (*null)(const char *key, void *user);
    void (*sint)(const char *key, int64_t value, void *user);
    void (*uint)(const char *key, uint64_t value, void *user);
    void (*f32)(const char *key, float value, void *user);
    void (*f64)(const char *key, double value, void *user);
    void (*key)(const char *key, const char *str, void *user);
    void (*string)(const char *key, const char *str, flexi_ssize_t len,
        void *user);
    void (*map_begin)(const char *key, flexi_ssize_t len, void *user);
    void (*map_end)(void *user);
    void (*vector_begin)(const char *key, flexi_ssize_t len, void *user);
    void (*vector_end)(void *user);
    void (*typed_vector)(const char *key, const void *ptr, flexi_type_e type,
        int width, flexi_ssize_t count, void *user);
    void (*blob)(const char *key, const void *ptr, flexi_ssize_t len,
        void *user);
    void (*boolean)(const char *key, bool val, void *user);
} flexi_parser_s;

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

/******************************************************************************/

/**
 * @brief Representation of a single value on the stack.
 */
typedef struct flexi_value_s {
    union {
        int64_t s64;
        uint64_t u64;
        float f32;
        double f64;
        flexi_ssize_t offset;
    } u;
    const char *key;
    flexi_type_e type;
    int width;
} flexi_value_s;

/**
 * @brief A function which, when called, returns a pointer to the stack
 *        value located at offset.
 *
 * @note The pointer returned by this function is not assumed to be stable
 *       after calling flexi_stack_push_fn.
 *
 * @param[in] offset Offset to obtain stack value for.
 * @param[in,out] user Pointer to user-specific data.
 * @return Pointer to flexi_value_s found at stack index, or NULL on failure.
 */
typedef flexi_value_s *(*flexi_stack_at_fn)(flexi_ssize_t offset, void *user);

/**
 * @brief Get current number of items in the stack.
 *
 * @param[in,out] user Pointer to user-specific data.
 * @return Number of items in stack.
 */
typedef flexi_ssize_t (*flexi_stack_count_fn)(void *user);

/**
 * @brief A function which, when called, pushes an empty value onto the stack
 *        and returns a pointer to it.
 *
 * @note The returned value does not need to be initialized, and might contain
 *       stale data.
 *
 * @param[in,out] user Pointer to user-specific data.
 * @return Pointer to fresh value, or NULL if a new value could not be pushed.
 */
typedef flexi_value_s *(*flexi_stack_push_fn)(void *user);

/**
 * @brief A function which, when called, pops the count number of values from
 *        the stack.
 *
 * @param[in] count Number of items to pop from the stack.
 * @param[in,out] user Pointer to user-specific data.
 * @return Number of items popped.
 */
typedef flexi_ssize_t (*flexi_stack_pop_fn)(flexi_ssize_t count, void *user);

/**
 * @brief This is an abstract stack interface used by flexi_writer_s in order
 *        to store data to be written to a FlexBuffer.  It can be implemented
 *        as a fixed-size stack, or as a dynamically-growing stack.
 */
typedef struct flexi_stack_s {
    flexi_stack_at_fn at;
    flexi_stack_count_fn count;
    flexi_stack_push_fn push;
    flexi_stack_pop_fn pop;
    void *user;
} flexi_stack_s;

/**
 * @brief A function which, when called, write data to a stream.
 */
typedef bool (*flexi_ostream_write_fn)(const void *ptr, flexi_ssize_t len,
    void *user);

/**
 * @brief A function which, when called, obtain data at the given index of
 *        the stream.
 */
typedef const void *(*flexi_ostream_data_at_fn)(flexi_ssize_t index,
    void *user);

/**
 * @brief A function which, when called, returns the current position of
 *        the stream.
 */
typedef bool (*flexi_ostream_tell_fn)(flexi_ssize_t *offset, void *user);

/**
 * @brief This is an output stream to write to.  This isn't really a stream
 *        in the traditional sense - it assumes random access, which means
 *        that you cannot reasonably write to anything other than a memory
 *        buffer.
 */
typedef struct flexi_ostream_s {
    flexi_ostream_write_fn write;
    flexi_ostream_data_at_fn data_at;
    flexi_ostream_tell_fn tell;
    void *user;
} flexi_ostream_s;

/**
 * @brief A writing interface for writing a FlexBuffer.
 */
typedef struct flexi_writer_s {
    flexi_stack_s stack;
    flexi_ostream_s ostream;
    flexi_result_e err;
} flexi_writer_s;

/**
 * @brief Create a stack struct from its constituent pieces.
 */
flexi_stack_s
flexi_make_stack(flexi_stack_at_fn at, flexi_stack_count_fn count,
    flexi_stack_push_fn push, flexi_stack_pop_fn pop, void *user);

/**
 * @brief Create an ostream struct from its constituent pieces.
 */
flexi_ostream_s
flexi_make_ostream(flexi_ostream_write_fn write,
    flexi_ostream_data_at_fn data_at, flexi_ostream_tell_fn tell, void *user);

/**
 * @brief Create a writer struct from its constituent pieces.
 */
flexi_writer_s
flexi_make_writer(const flexi_stack_s *stack, const flexi_ostream_s *ostream);

/**
 * @brief Push a null value to the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @return FLEXI_OK || FLEXI_ERR_BADSTACK.
 */
flexi_result_e
flexi_write_null(flexi_writer_s *writer, const char *key);

/**
 * @brief Push a signed int value to the stack.
 *
 * @note When the int is later written, it will only take up as much space
 *       as necessary to fully fit the value.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to push to the stack.
 * @return FLEXI_OK || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_sint(flexi_writer_s *writer, const char *key, int64_t val);

/**
 * @brief Push an unsigned int value to the stack.
 *
 * @note When the int is later written, it will only take up as much space
 *       as necessary to fully fit the value.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] value Value to push to the stack.
 * @return FLEXI_OK || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_uint(flexi_writer_s *writer, const char *key, uint64_t value);

/**
 * @brief Push a 32-bit float value to the stack.
 *
 * @note If this float value is placed into a map or vector with a stride
 *       of 8 bytes, it will be promoted to a 64-bit float.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to push to the stack.
 * @return FLEXI_OK || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_f32(flexi_writer_s *writer, const char *key, float val);

/**
 * @brief Push a 64-bit float value to the stack.
 *
 * @note If this float value is placed into a map or vector with a stride
 *       of 4 bytes, it will be narrowed to a 32-bit float.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to push to the stack.
 * @return FLEXI_OK || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_f64(flexi_writer_s *writer, const char *key, double val);

/**
 * @brief Write a key value to the stream, then push an offset to that key
 *        onto the stack.
 *
 * @note Keys are used when creating keysets for a map.  Because they lack
 *       a length, there isn't much of a reason to use them outside of that
 *       specific use case, so this function lacks a way to pair them with
 *       a key.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] str Key to write and push to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_key(flexi_writer_s *writer, const char *str);

/**
 * @brief Write a key value to the stream, then push an offset to that key
 *        onto the stack.
 *
 * @note While allowed by the protocol, there's almost never a good reason
 *       to put a key directly into a vector or map.  Prefer strings instead.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] str Key to write and push to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_keyed_key(flexi_writer_s *writer, const char *key, const char *str);

/**
 * @brief Write a string value to the stream, then push an offset to that
 *        string onto the stack.
 *
 * @note The written string value is prefixed with its length and has a '\0'
 *       appended to the end.  By convention, strings are assumed to be UTF-8,
 *       although this function does no uncode validation.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the string is to be inserted into a map, or
 *                NULL if the string will not be used in a map.
 * @param[in] str String to write and push to the stack.
 * @param[in] len Length of the string to be written to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_string(flexi_writer_s *writer, const char *key, const char *str,
    size_t len);

/**
 * @brief Write a null-terminated string value to the stream, then push an
 *        offset to that string onto the stack.
 *
 * @note This is just a convenience function for writing a null-terminated
 *       string, it has the same underlying representation on the wire.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the string is to be inserted into a map, or
 *                NULL if the string will not be used in a map.
 * @param[in] str String to write and push to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_strlen(flexi_writer_s *writer, const char *key, const char *str);

/**
 * @brief Write an indirect signed integer to the stream, then push an offset
 *        to that value onto the stack.
 *
 * @note This is useful if you need to write a large number to a map or vector
 *       with a small stride.  However, it is slower to access than the direct
 *       counterpart.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to write and push to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_indirect_sint(flexi_writer_s *writer, const char *key, int64_t val);

/**
 * @brief Write an indirect unsigned integer to the stream, then push an
 *        offset to that value onto the stack.
 *
 * @note This is useful if you need to write a large number to a map or vector
 *       with a small stride.  However, it is slower to access than the direct
 *       counterpart.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to write and push to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_indirect_uint(flexi_writer_s *writer, const char *key,
    uint64_t val);

/**
 * @brief Write an indirect 32-bit float to the stream, then push an
 *        offset to that value onto the stack.
 *
 * @note This is useful if you need to write a float to a map or vector with
 *       a small stride.  However, it is slower to access than the direct
 *       counterpart.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to write and push to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_indirect_f32(flexi_writer_s *writer, const char *key, float val);

/**
 * @brief Write an indirect 64-bit float to the stream, then push an
 *        offset to that value onto the stack.
 *
 * @note This is useful if you need to write a float to a map or vector with
 *       a small stride.  However, it is slower to access than the direct
 *       counterpart.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to write and push to the stack.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_indirect_f64(flexi_writer_s *writer, const char *key, double val);

/**
 * @brief Writes a vector of key offsets to the stream.  Pops `len` keys
 *        from the stack, pushes a single vector of keys to the stack, and
 *        hands you back a reference to that keyset which you can pass to
 *        future calls to flexi_write_map.
 *
 * @note FlexBuffer maps are not hashtables, they consist of a vector of keys
 *       and a vector of values.  The vector of keys is sorted, so they can
 *       be looked up via binary search.  A single vector of keys can be
 *       used by multiple maps.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] len Number of keys on the stack to collect into a map key
 *                vector.
 * @param[in] stride Desired stride of the vector.  If the stride is too small
 *                   to fit one of the offsets, it will be widened to fit.
 * @param[out] keyset Opaque handle to keyset, which can be passed a future
 *                    map.
 * @return FLEXI_OK || FLEXI_ERR_BADSTACK || FLEXI_ERR_NOTKEYS ||
 *         FLEXI_ERR_BADWRITE || FLEXI_ERR_BADSTACK.
 */
flexi_result_e
flexi_write_map_keys(flexi_writer_s *writer, flexi_ssize_t len,
    flexi_width_e stride, flexi_stack_idx_t *keyset);

/**
 * @brief Writes a vector of map values to the stream.  Pops `len` values
 *        from the stack, and pushes a single vector of map values to the
 *        stack.
 *
 * @notes This is the "vector of values" half of a map.  You need to have
 *        created the keys first.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if this map is going to be nested in another
 *                map, or NULL if the value will not be used in a map.
 * @param[in] keyset Stack index pointing to vector of keys to use for map.
 * @param[in] len Number of values on the stack to use for the map.
 * @param[in] stride Desired stride of the map.  If the stride is too small
 *                   to fit one of the values, it will be widened to fit.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_ERR_BADSTACK.
 */
flexi_result_e
flexi_write_map_values(flexi_writer_s *writer, const char *key,
    flexi_stack_idx_t keyset, flexi_ssize_t len, flexi_width_e stride);

/**
 * @brief Write a map to the stream using keys of pushed values.  Pops `len`
 *        values from the stack and pushes a single vector of map values
 *        to the stack.
 *
 * @note This function is best for "one shot" maps where you're not going
 *       to be reusing keys.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if this map is going to be nested in another
 *                map, or NULL if the value will not be used in a map.
 * @param[in] len Number of values on the stack to use for the map.
 * @param[in] stride Desired stride of the map.  If the stride is too small
 *                   to fit one of the values, it will be widened to fit.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_ERR_BADSTACK.
 */
flexi_result_e
flexi_write_map(flexi_writer_s *writer, const char *key, flexi_ssize_t len,
    flexi_width_e stride);

/**
 * @brief Write a vector to the stream.  Pops `len` values from the stack
 *        and pushes a single vector to the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] len Number of values on the stack to use for vector.
 * @param[in] stride Desired width of vector.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE || FLEXI_ERR_BADSTACK.
 */
flexi_result_e
flexi_write_vector(flexi_writer_s *writer, const char *key, flexi_ssize_t len,
    flexi_width_e stride);

/**
 * @brief Write a typed vector of signed ints to the stream.  Pushes a single
 *        vector to the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to int array to be written.
 * @param[in] stride Width of each individual int.
 * @param[in] len Number of elements in the array.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_typed_vector_sint(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, flexi_ssize_t len);

/**
 * @brief Write a typed vector of unsigned ints to the stream.  Pushes a single
 *        vector to the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to int array to be written.
 * @param[in] stride Width of each individual int.
 * @param[in] len Number of elements in the array.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_typed_vector_uint(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, flexi_ssize_t len);

/**
 * @brief Write a typed vector of floats ints to the stream.  Pushes a single
 *        vector to the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to float array to be written.
 * @param[in] stride Width of each individual float.
 * @param[in] len Number of elements in the array.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_typed_vector_flt(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, flexi_ssize_t len);

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
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_blob(flexi_writer_s *writer, const char *key, const void *ptr,
    flexi_ssize_t len, int align);

/**
 * @brief Push a boolean value to the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key to use if the value is to be inserted into a map, or
 *                NULL if the value will not be used in a map.
 * @param[in] val Value to push to the stack.
 * @return FLEXI_OK || FLEXI_BADSTACK.
 */
flexi_result_e
flexi_write_bool(flexi_writer_s *writer, const char *key, bool val);

/**
 * @brief Write a typed vector of boleans ints to the stream.  Pushes a single
 *        vector to the stack.
 *
 * @param[in,out] writer Writer to operate on.
 * @param[in] key Key for use in a map.  NULL if there is no key.
 * @param[in] ptr Pointer to int array to be written.
 * @param[in] stride Width of each individual int.
 * @param[in] len Number of elements in the array.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_typed_vector_bool(flexi_writer_s *writer, const char *key,
    const bool *ptr, flexi_ssize_t len);

/**
 * @brief Pops a single value from the stack and writes it out to the stream
 *        as the root of the message.  The message is considered "done" at
 *        this point.
 *
 * @param[in,out] writer Writer to operate on.
 * @return FLEXI_OK || FLEXI_ERR_BADSTACK || FLEXI_ERR_BADWRITE.
 */
flexi_result_e
flexi_write_finalize(flexi_writer_s *writer);

/**
 * @brief Peek at the given stack value.
 *
 * @note This information is for error reporting purposes.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] offset Stack offset to examine, starting from the bottom.
 * @param[out] value Pointer to value on the stack.  Not touched on error.
 * @return FLEXI_OK || FLEXI_ERR_BADSTACK.
 */
flexi_result_e
flexi_writer_debug_stack_at(const flexi_writer_s *writer, flexi_ssize_t offset,
    flexi_value_s **value);

/**
 * @brief Return the number of items on the stack.
 *
 * @note This information is for error reporting purposes.
 *
 * @param[in] writer Writer to operate on.
 * @return Number of items currently in the stack.
 */
flexi_ssize_t
flexi_writer_debug_stack_count(flexi_writer_s *writer);

#if FLEXI_CPLUSPLUS
}
#endif

#if FLEXI_CPLUSPLUS

#if FLEXI_CPLUSPLUS >= 201703L

#include <string_view>

inline flexi_result_e
flexi_write_string(flexi_writer_s *writer, const char *key,
    std::string_view str)
{
    return flexi_write_string(writer, key, str.data(), str.length());
}

#endif

template<typename T> inline flexi_result_e
flexi_write_typed_vector(flexi_writer_s *writer, const char *key, const T *arr,
    flexi_ssize_t len) FLEXI_CPP11_EQ_DELETE;

template<> inline flexi_result_e
flexi_write_typed_vector<int8_t>(flexi_writer_s *writer, const char *key,
    const int8_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_sint(writer, key, arr, FLEXI_WIDTH_1B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<int16_t>(flexi_writer_s *writer, const char *key,
    const int16_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_sint(writer, key, arr, FLEXI_WIDTH_2B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<int32_t>(flexi_writer_s *writer, const char *key,
    const int32_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_sint(writer, key, arr, FLEXI_WIDTH_4B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<int64_t>(flexi_writer_s *writer, const char *key,
    const int64_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_sint(writer, key, arr, FLEXI_WIDTH_8B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint8_t>(flexi_writer_s *writer, const char *key,
    const uint8_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_uint(writer, key, arr, FLEXI_WIDTH_1B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint16_t>(flexi_writer_s *writer, const char *key,
    const uint16_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_uint(writer, key, arr, FLEXI_WIDTH_2B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint32_t>(flexi_writer_s *writer, const char *key,
    const uint32_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_uint(writer, key, arr, FLEXI_WIDTH_4B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<uint64_t>(flexi_writer_s *writer, const char *key,
    const uint64_t *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_uint(writer, key, arr, FLEXI_WIDTH_8B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<float>(flexi_writer_s *writer, const char *key,
    const float *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_flt(writer, key, arr, FLEXI_WIDTH_4B, len);
}

template<> inline flexi_result_e
flexi_write_typed_vector<double>(flexi_writer_s *writer, const char *key,
    const double *arr, flexi_ssize_t len)
{
    return flexi_write_typed_vector_flt(writer, key, arr, FLEXI_WIDTH_8B, len);
}

#endif // #if FLEXI_CPLUSPLUS
