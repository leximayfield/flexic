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

#include <string.h>

/******************************************************************************/

#ifndef FLEXI_CONFIG_MAX_DEPTH
/**
 * @brief The maximum number of nested vectors or maps before the parse fails.
 *
 * @details A maliciously-formed FlexBuffer could nest vectors inside vectors
 *          and crash the parser with a stack overflow.  This limit prevents
 *          excessive nesting which would result in such a crash.
 */
#define FLEXI_CONFIG_MAX_DEPTH (32)
#endif

#ifndef FLEXI_CONFIG_MAX_ITERABLES
/**
 * @brief The maximum number of non-typed vectors and maps to parse before
 *        failing.
 *
 * @details FlexBuffers allow for a single value to be referenced from multiple
 *          places in the message.  However, this feature has the potential
 *          for misuse in maliciously-designed "FlexBuffer bomb" inputs where
 *          iterable containers are shared and nested in ways that take a
 *          long time to parse.
 *
 *          Note that this limit does not count typed vectors, as these do
 *          not allow nesting other iterables inside them.
 */
#define FLEXI_CONFIG_MAX_ITERABLES (2048)
#endif

#define ASSERT(cond) (void)0

/******************************************************************************/

#define MIN(a, b) ((b) < (a) ? (b) : (a))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define COUNTOF(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SINT_WIDTH(v)                                                          \
    ((v) <= INT8_MAX && (v) >= INT8_MIN         ? 1                            \
        : (v) <= INT16_MAX && (v) >= INT16_MIN  ? 2                            \
        : (v) <= UINT32_MAX && (v) >= INT32_MIN ? 4                            \
                                                : 8);
#define UINT_WIDTH(v)                                                          \
    ((v) <= UINT8_MAX ? 1 : (v) <= UINT16_MAX ? 2 : (v) <= UINT32_MAX ? 4 : 8);

#define UNPACK_TYPE(packed) ((flexi_type_e)(packed >> 2))
#define UNPACK_WIDTH(packed) ((flexi_width_e)(packed & 0x03))
#define WIDTH_TO_BYTES(width) ((int)(1 << width))

#define PACK_TYPE(t) ((flexi_packed_t)(t) << 2)
#define PACK_WIDTH(w)                                                          \
    ((w) == 1      ? (flexi_packed_t)(0)                                       \
        : (w) == 2 ? (flexi_packed_t)(1)                                       \
        : (w) == 4 ? (flexi_packed_t)(2)                                       \
                   : (flexi_packed_t)(3))

typedef struct parse_limits_s {
    int depth;
    int iterables;
} parse_limits_s;

static flexi_result_e
parse_cursor(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user, parse_limits_s *limits);

/**
 * @brief Checked multiply.
 */
static bool
checked_mul(uint64_t *z, uint64_t x, uint64_t y)
{
    *z = x * y;
    return !(x && *z / x != y);
}

/**
 * @brief Returns true if the value has a single bit set, making it a
 *        power-of-two.
 */
static bool
has_single_bit32(uint32_t x)
{
    return x && !(x & (x - 1));
}

/**
 * @brief Round value to the nearest multiple.  Multiple must be a power
 *        of two.
 *
 * @param x Value to round.
 * @param m Multiple.
 * @return Rounded multiple.
 */
static uint64_t
round_to_pow2_mul64(uint64_t x, uint64_t m)
{
    return (x + (m - 1)) & ~(m - 1);
}

/**
 * @brief Return true if the type is any signed integer type.
 */
static bool
type_is_sint(flexi_type_e type)
{
    if (type == FLEXI_TYPE_SINT || type == FLEXI_TYPE_INDIRECT_SINT) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type is any unsigned integer type.
 */
static bool
type_is_uint(flexi_type_e type)
{
    if (type == FLEXI_TYPE_UINT || type == FLEXI_TYPE_INDIRECT_UINT) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type is any int type.
 */
static bool
type_is_anyint(flexi_type_e type)
{
    switch (type) {
    case FLEXI_TYPE_SINT:
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_SINT:
    case FLEXI_TYPE_INDIRECT_UINT: return true;
    default: return false;
    }
}

/**
 * @brief Return true if the type is any float type.
 */
static bool
type_is_flt(flexi_type_e type)
{
    if (type == FLEXI_TYPE_FLOAT || type == FLEXI_TYPE_INDIRECT_FLOAT) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type can be written directly, without an offset.
 */
static bool
type_is_direct(flexi_type_e type)
{
    if (type >= FLEXI_TYPE_NULL && type <= FLEXI_TYPE_FLOAT) {
        return true;
    } else if (type == FLEXI_TYPE_BOOL) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type can be written indirectly, with an offset.
 */
static bool
type_is_indirect(flexi_type_e type)
{
    if (type >= FLEXI_TYPE_KEY && type <= FLEXI_TYPE_BLOB) {
        return true;
    } else if (type == FLEXI_TYPE_VECTOR_BOOL) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type is a vector type.  Types that are vector-like
 *        don't count.
 */
static bool
type_is_vector(flexi_type_e type)
{
    if (type >= FLEXI_TYPE_VECTOR && type <= FLEXI_TYPE_VECTOR_FLOAT4) {
        return true;
    } else if (type == FLEXI_TYPE_VECTOR_BOOL) {
        return true;
    }
    return false;
}

/**
 * @brief Returns true if read would be valid for the given source and width.
 */
bool
read_is_valid(const flexi_buffer_s *buffer, const char *src, int width)
{
    return src >= buffer->data &&
           src + width - 1 < buffer->data + buffer->length;
}

/**
 * @brief Safely seek backwards in the buffer.
 *
 * @param [in] buffer Buffer to constrain offset resolution.
 * @param [in] src Starting point for offset.
 * @param [in] offset Extent of offset in the negative direction.
 * @param [out] dest Output destination value.
 * @return True if offset was resolved.
 */
static bool
buffer_seek_back(const flexi_buffer_s *buffer, const char *src, size_t offset,
    const char **dest)
{
    if (offset == 0) {
        // An offset of 0 means someone is being sneaky.
        return false;
    }

    size_t max_offset = src - buffer->data;
    if (offset > max_offset) {
        // We would fall off the front of the buffer.
        return false;
    }

    *dest = src - offset;
    return true;
}

/**
 * @brief Read an unsigned integer from the passed buffer of at most size_t
 *        width.
 *
 * @note Values of width < size bytes will be promoted to size_t.
 *
 * @pre Width must be a valid width in bytes.
 *
 * @param[in] buffer Buffer to constrain read inside.
 * @param[in] src Pointer to data to read from.
 * @param[in] width Width of data to read.
 * @param[out] dst Output value.
 * @return True if read was successful, false if read would have gone out
 *         of range of the buffer.
 */
static bool
buffer_read_size_by_width(const flexi_buffer_s *buffer, const char *src,
    int width, size_t *dst)
{
    if (!read_is_valid(buffer, src, width)) {
        return false;
    }

    switch (width) {
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
#if (SIZE_MAX == UINT32_MAX)
        *dst = (size_t)v;
        return v <= SIZE_MAX;
#else
        *dst = v;
        return true;
#endif
    }
    }
    return false;
}

/**
 * @brief Read a signed integer from the passed buffer.
 *
 * @note Values of width < 8 bytes will be promoted to int64_t.
 *
 * @param[in] buffer Buffer to constrain read inside.
 * @param[in] src Pointer to data to read from.
 * @param[in] width Width of data to read.
 * @param[out] dst Output value.
 * @return True if read was successful.
 */
static bool
buffer_read_sint(const flexi_buffer_s *buffer, const char *src, int width,
    int64_t *dst)
{
    if (!read_is_valid(buffer, src, width)) {
        return false;
    }

    switch (width) {
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

/**
 * @brief Read an unsigned integer from the passed buffer.
 *
 * @note Values of width < 8 bytes will be promoted to uint64_t.
 *
 * @pre Width must be a valid width in bytes.
 *
 * @param[in] buffer Buffer to constrain read inside.
 * @param[in] src Pointer to data to read from.
 * @param[in] width Width of data to read.
 * @param[out] dst Output value.
 * @return True if read was successful, false if read would have gone out
 *         of range of the buffer.
 */
static bool
buffer_read_uint(const flexi_buffer_s *buffer, const char *src, int width,
    uint64_t *dst)
{
    if (!read_is_valid(buffer, src, width)) {
        return false;
    }

    switch (width) {
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

/**
 * @brief Read a 4-byte float from the passed buffer.
 *
 * @note Floats of width != 4 bytes will be converted.
 *
 * @param[in] buffer Buffer to constrain read inside.
 * @param[in] src Pointer to data to read from.
 * @param[in] width Width of data to read.
 * @param[out] dst Output value.
 * @return True if read was successful.
 */
static bool
buffer_read_f32(const flexi_buffer_s *buffer, const char *src, int width,
    float *dst)
{
    if (!read_is_valid(buffer, src, width)) {
        return false;
    }

    switch (width) {
    case 4: {
        float v;
        memcpy(&v, src, 4);
        *dst = v;
        return true;
    }
    case 8: {
        double v;
        memcpy(&v, src, 8);
        *dst = (float)v;
        return true;
    }
    }
    return false;
}

/**
 * @brief Read an 8-byte float from the passed buffer.
 *
 * @note Floats of width != 8 bytes will be converted.
 *
 * @param[in] buffer Buffer to constrain read inside.
 * @param[in] src Pointer to data to read from.
 * @param[in] width Width of data to read.
 * @param[out] dst Output value.
 * @return True if read was successful.
 */
static bool
buffer_read_f64(const flexi_buffer_s *buffer, const char *src, int width,
    double *dst)
{
    if (!read_is_valid(buffer, src, width)) {
        return false;
    }

    switch (width) {
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

/**
 * @brief Write exacty len bytes and return true if the write was complete.
 *
 * @param[in] writer Writer to write to.
 * @param[in] ptr Pointer to data to write.
 * @param[in] len Length of data to write.
 * @return True if write was complete.
 */
static bool
writer_write(flexi_writer_s *writer, const void *ptr, size_t len)
{
    return writer->write_func(ptr, len, writer->user);
}

/**
 * @brief Return a pointer to already-written data, by absolute offset.
 *
 * @param[in] writer Writer to query.
 * @param[in] offset Offset to find.
 * @return Pointer to data at offset.
 */
static const void *
writer_data_at(flexi_writer_s *writer, size_t offset)
{
    return writer->data_at_func(offset, writer->user);
}

/**
 * @brief Return the position of the stream.
 *
 * @param[in] writer Writer to query.
 * @param[out] offset Position of the stream.
 * @return True if stream was in a valid state and returned an offset.
 */
static bool
writer_tell(flexi_writer_s *writer, size_t *offset)
{
    return writer->tell_func(offset, writer->user);
}

/**
 * @brief Align stream to nearest multiple of width.
 *
 * @param[in] writer Writer to align.
 * @param[in] prefix Number of bytes before the address we want to align.
 * @param[in] width Width to align to.
 * @param[out] offset Position of the stream.
 * @return True if stream was in a valid state and returned an offset.
 */
static bool
write_padding(flexi_writer_s *writer, int prefix, int width, size_t *offset)
{
    if (!has_single_bit32((uint32_t)width)) {
        return false;
    }

    size_t src_offset;
    if (!writer->tell_func(&src_offset, writer->user)) {
        return false;
    }

    // Round offset to nearest multiple.
    size_t dst_offset = round_to_pow2_mul64(src_offset, (uint64_t)width);
    ptrdiff_t padding_len = (ptrdiff_t)(dst_offset - src_offset);

    // Loop, writing 8 bytes at a time until we're done.
    static const char s_padding[8] = {0};
    for (; padding_len > 0; padding_len -= 8) {
        size_t write_len = MIN((size_t)padding_len, 8);
        if (!writer->write_func(s_padding, write_len, writer->user)) {
            return false;
        }
    }

    *offset = (size_t)prefix + dst_offset;
    return true;
}

/**
 * @brief Write a signed integer to the passed writer.
 *
 * @note Values will first be converted to the equivalent integer of the
 *       passed width.  Values that are too large or small for the passed
 *       width will not be truncated, but will return a failure state.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] v Value to write.
 * @param[in] width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool
write_sint_by_width(flexi_writer_s *writer, int64_t v, int width)
{
    ASSERT(width == 1 || width == 2 || width == 4 || width == 8);

    switch (width) {
    case 1: {
        if (v > INT8_MAX || v < INT8_MIN) {
            return false;
        }

        int8_t vv = (int8_t)v;
        return writer_write(writer, &vv, sizeof(int8_t));
    }
    case 2: {
        if (v > INT16_MAX || v < INT16_MIN) {
            return false;
        }

        int16_t vv = (int16_t)v;
        return writer_write(writer, &vv, sizeof(int16_t));
    }
    case 4: {
        if (v > INT32_MAX || v < INT32_MIN) {
            return false;
        }

        int32_t vv = (int32_t)v;
        return writer_write(writer, &vv, sizeof(int32_t));
    }
    case 8: return writer_write(writer, &v, sizeof(int64_t));
    }
    return false;
}

/**
 * @brief Write an unsigned integer to the passed writer.
 *
 * @note Values will first be converted to the equivalent integer of the
 *       passed width.  Values that are too large or small for the passed
 *       width will not be truncated, but will return a failure state.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] v Value to write.
 * @param[in] width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool
write_uint_by_width(flexi_writer_s *writer, uint64_t v, int width)
{
    ASSERT(width == 1 || width == 2 || width == 4 || width == 8);

    switch (width) {
    case 1: {
        if (v > UINT8_MAX) {
            return false;
        }

        uint8_t vv = (uint8_t)v;
        return writer_write(writer, &vv, sizeof(uint8_t));
    }
    case 2: {
        if (v > UINT16_MAX) {
            return false;
        }

        uint16_t vv = (uint16_t)v;
        return writer_write(writer, &vv, sizeof(uint16_t));
    }
    case 4: {
        if (v > UINT32_MAX) {
            return false;
        }

        uint32_t vv = (uint32_t)v;
        return writer_write(writer, &vv, sizeof(uint32_t));
    }
    case 8: return writer_write(writer, &v, sizeof(uint64_t));
    }
    return false;
}

/**
 * @brief Write a 4-byte float to the passed writer.
 *
 * @note Floats of width != 4 bytes will be converted.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] v Value to write.
 * @param[in] width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool
write_f32(flexi_writer_s *writer, float v, int width)
{
    ASSERT(width == 4 || width == 8);

    switch (width) {
    case 4: return writer_write(writer, &v, sizeof(float));
    case 8: {
        double vv = v;
        return writer_write(writer, &vv, sizeof(double));
    }
    }
    return false;
}

/**
 * @brief Write an 8-byte float to the passed writer.
 *
 * @note Floats of width != 8 bytes will be converted.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] v Value to write.
 * @param[in] width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool
write_f64(flexi_writer_s *writer, double v, int width)
{
    ASSERT(width == 4 || width == 8);

    switch (width) {
    case 4: {
        float vv = (float)v;
        return writer_write(writer, &vv, sizeof(float));
    }
    case 8: return writer_write(writer, &v, sizeof(double));
    }
    return false;
}

/**
 * @brief Given a cursor pointing at the base of a vector or vector-like
 *        type, obtain its length.
 *
 * @param[in] cursor Cursor to check.
 * @param[out] len Length of vector.
 * @return True if length was found.
 */
static bool
cursor_get_len(const flexi_cursor_s *cursor, size_t *len)
{
    ASSERT(cursor->type == FLEXI_TYPE_STRING ||
           cursor->type == FLEXI_TYPE_MAP ||
           cursor->type == FLEXI_TYPE_VECTOR ||
           cursor->type == FLEXI_TYPE_VECTOR_SINT ||
           cursor->type == FLEXI_TYPE_VECTOR_UINT ||
           cursor->type == FLEXI_TYPE_VECTOR_FLOAT ||
           cursor->type == FLEXI_TYPE_BLOB ||
           cursor->type == FLEXI_TYPE_VECTOR_BOOL);

    return buffer_read_uint(&cursor->buffer, cursor->cursor - cursor->width,
        cursor->width, len);
}

/**
 * @brief Given a cursor pointing at the base of a vector which contains
 *        trailing types, return a pointer to the first type.
 *
 * @warning This is a bug-prone function, be careful when working on it.
 *
 * @param[in] cursor Cursor to check.
 * @param[out] packed Pointer to first packed type.
 * @return True if types were located.
 */
static bool
cursor_vector_types(const flexi_cursor_s *cursor, const flexi_packed_t **packed)
{
    ASSERT(cursor->type == FLEXI_TYPE_MAP || cursor->type == FLEXI_TYPE_VECTOR);

    size_t len = 0;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    // Don't wrap the offset calculation.
    size_t offset;
    if (!checked_mul(&offset, len, cursor->width)) {
        return false;
    }

    // Make sure the destination pointer is in-bounds at all valid offsets.
    const char *dest = cursor->cursor + offset;
    if (dest + len >= cursor->buffer.data + cursor->buffer.length) {
        return false;
    }

    *packed = (const flexi_packed_t *)dest;
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a vector which contains
 *        trailing types, return a cursor pointing at the value at the given
 *        index.
 *
 * @param[in] cursor Cursor to use as base.
 * @param[in] index Index to look up.
 * @param[out] dest Destination cursor.
 * @return True if seek was successful.
 */
static bool
cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index,
    flexi_cursor_s *dest)
{
    ASSERT(cursor->type == FLEXI_TYPE_MAP || cursor->type == FLEXI_TYPE_VECTOR);

    const flexi_packed_t *types = NULL;
    if (!cursor_vector_types(cursor, &types)) {
        return false;
    }

    flexi_type_e type = UNPACK_TYPE(types[index]);
    if (type_is_direct(type)) {
        // No need to resolve an offset, we're pretty much done.
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + (index * (size_t)cursor->width);
        dest->type = type;
        dest->width = cursor->width;
        return true;
    }

    size_t offset = 0;
    const char *offset_ptr = cursor->cursor + (index * (size_t)cursor->width);
    if (!buffer_read_size_by_width(&cursor->buffer, offset_ptr, cursor->width,
            &offset)) {
        return false;
    }

    if (!buffer_seek_back(&cursor->buffer, offset_ptr, offset, &dest->cursor)) {
        return false;
    }

    dest->buffer = cursor->buffer;
    dest->type = type;
    dest->width = WIDTH_TO_BYTES(UNPACK_WIDTH(types[index]));
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a map, return the map key
 *        used by that index.
 *
 * @param[in] cursor Cursor to use as base.
 * @param[in] index Index to look up.
 * @param[out] str Key found by lookup.
 * @return True if lookup was successful.
 */
static bool
cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index,
    const char **str)
{
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    // Figure out offset and width of keys vector.
    uint64_t keys_offset, keys_width;
    if (!buffer_read_uint(&cursor->buffer, cursor->cursor - (cursor->width * 3),
            cursor->width, &keys_offset)) {
        return false;
    }

    if (!buffer_read_uint(&cursor->buffer, cursor->cursor - (cursor->width * 2),
            cursor->width, &keys_width)) {
        return false;
    }

    // Seek the keys base.
    const char *keys_base =
        cursor->cursor - ((3 * cursor->width) + keys_offset);

    // Resolve the key.
    size_t offset = 0;
    const char *offset_ptr = keys_base + (index * (size_t)keys_width);
    if (!buffer_read_size_by_width(&cursor->buffer, offset_ptr, (int)keys_width,
            &offset)) {
        return false;
    }

    return buffer_seek_back(&cursor->buffer, offset_ptr, offset, str);
}

/**
 * @brief Seek a map key via linear search.
 *
 * @param[in] cursor Cursor of map to use as base.
 * @param[in] len Length of map at cursor.
 * @param[in] key Key to check for.
 * @param[out] dest Destination cursor.
 * @return FLEXI_OK Key was found.
 * @return FLEXI_ERR_NOTFOUND Key was not found.
 * @return FLEXI_ERR_BADREAD Read error while looking up key.
 */
static flexi_result_e
cursor_seek_map_key_linear(const flexi_cursor_s *cursor, size_t len,
    const char *key, flexi_cursor_s *dest)
{
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    // Linear search.
    for (size_t i = 0; i < len; i++) {
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, i, &cmp)) {
            return FLEXI_ERR_BADREAD;
        }

        if (!strcmp(cmp, key)) {
            return cursor_seek_vector_index(cursor, i, dest)
                       ? FLEXI_OK
                       : FLEXI_ERR_BADREAD;
        }
    }

    return FLEXI_ERR_NOTFOUND;
}

/**
 * @brief Seek a map key via binary search.
 *
 * @param[in] cursor Cursor of map to use as base.
 * @param[in] len Length of map at cursor.
 * @param[in] key Key to check for.
 * @param[out] dest Destination cursor.
 * @return FLEXI_OK Key was found.
 * @return FLEXI_ERR_NOTFOUND Key was not found.
 * @return FLEXI_ERR_BADREAD Read error while looking up key.
 */
static flexi_result_e
cursor_seek_map_key_bsearch(const flexi_cursor_s *cursor, size_t len,
    const char *key, flexi_cursor_s *dest)
{
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    size_t left = 0;
    size_t right = len - 1;
    while (left <= right) {
        size_t i = left + ((right - left) / 2);
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, i, &cmp)) {
            return FLEXI_ERR_BADREAD;
        }

        int res = strcmp(cmp, key);
        if (res == 0) {
            return cursor_seek_vector_index(cursor, i, dest)
                       ? FLEXI_OK
                       : FLEXI_ERR_BADREAD;
        }

        if (res < 0) {
            left = i + 1;
        } else {
            right = i - 1;
        }
    }

    return FLEXI_ERR_NOTFOUND;
}

/**
 * @brief Call the signed integer callback.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_sint(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    int64_t v;
    if (!buffer_read_sint(&cursor->buffer, cursor->cursor, cursor->width, &v)) {
        return FLEXI_ERR_BADREAD;
    }

    parser->sint(key, v, user);
    return FLEXI_OK;
}

/**
 * @brief Call the unsigned integer callback.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_uint(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    uint64_t v;
    if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width, &v)) {
        return FLEXI_ERR_BADREAD;
    }

    parser->uint(key, v, user);
    return FLEXI_OK;
}

/**
 * @brief Call the float callback.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_flt(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    switch (cursor->width) {
    case 4: {
        float v;
        if (!buffer_read_f32(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->f32(key, v, user);
        return FLEXI_OK;
    }
    case 8: {
        double v;
        if (!buffer_read_f64(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->f64(key, v, user);
        return FLEXI_OK;
    }
    }
    return FLEXI_ERR_INTERNAL;
}

/**
 * @brief Call the string callback.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_string(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    parser->string(key, cursor->cursor, len, user);
    return FLEXI_OK;
}

/**
 * @brief Call the map callbacks and iterate map values.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @param[in] limits Parse limit tracking.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_map(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user, parse_limits_s *limits)
{
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    flexi_result_e res;
    parser->map_begin(key, len, user);
    for (size_t i = 0; i < len; i++) {
        const char *str;
        if (!cursor_map_key_at_index(cursor, i, &str)) {
            return FLEXI_ERR_BADREAD;
        }

        flexi_cursor_s value;
        if (!cursor_seek_vector_index(cursor, i, &value)) {
            return FLEXI_ERR_BADREAD;
        }
        limits->depth += 1;
        res = parse_cursor(parser, str, &value, user, limits);
        if (FLEXI_ERROR(res)) {
            return res;
        }
        limits->depth -= 1;
    }

    parser->map_end(user);
    return FLEXI_OK;
}

/**
 * @brief Call the vector callbacks and iterate vector values.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @param[in] limits Parse limit tracking.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 * @return FLEXI_ERR_PARSELIMIT Read hit a configured parse limit.
 */
static flexi_result_e
parser_emit_vector(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user, parse_limits_s *limits)
{
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    flexi_result_e res;
    parser->vector_begin(key, len, user);
    for (size_t i = 0; i < len; i++) {
        flexi_cursor_s value;
        if (!cursor_seek_vector_index(cursor, i, &value)) {
            return FLEXI_ERR_BADREAD;
        }
        limits->depth += 1;
        res = parse_cursor(parser, NULL, &value, user, limits);
        if (FLEXI_ERROR(res)) {
            return res;
        }
        limits->depth -= 1;
    }

    parser->vector_end(user);
    return FLEXI_OK;
}

/**
 * @brief Call the typed vector callback.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_typed_vector(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    parser->typed_vector(key, cursor->cursor, len, cursor->type, cursor->width,
        user);
    return FLEXI_OK;
}

/**
 * @brief Call the vector callbacks and iterate keys in vector.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_vector_keys(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    parser->vector_begin(key, len, user);
    for (size_t i = 0; i < len; i++) {
        // Access the offset by hand.
        size_t offset = 0;
        const char *offset_ptr = cursor->cursor + (i * (size_t)cursor->width);
        if (!buffer_read_size_by_width(&cursor->buffer, offset_ptr,
                cursor->width, &offset)) {
            return FLEXI_ERR_BADREAD;
        }

        const char *dest = NULL;
        if (!buffer_seek_back(&cursor->buffer, offset_ptr, offset, &dest)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->key(key, dest, user);
    }

    parser->vector_end(user);
    return FLEXI_OK;
}

/**
 * @brief Call the blob callback.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK Successful read.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 */
static flexi_result_e
parser_emit_vector_blob(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    parser->blob(key, cursor->cursor, len, user);
    return FLEXI_OK;
}

/**
 * @brief Call the appropriate parser callbacks at the given cursor.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of cursor.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @param[in] limits Parse limit tracking.
 * @return FLEXI_OK Successful parse.
 * @return FLEXI_ERR_BADREAD Read was invalid.
 * @return FLEXI_ERR_PARSELIMIT Read hit a configured parse limit.
 */
static flexi_result_e
parse_cursor(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user, parse_limits_s *limits)
{
    if (limits->depth >= FLEXI_CONFIG_MAX_DEPTH) {
        // Prevent stack overflows.
        return FLEXI_ERR_PARSELIMIT;
    }

    switch (cursor->type) {
    case FLEXI_TYPE_NULL: parser->null(key, user); return FLEXI_OK;
    case FLEXI_TYPE_SINT:
    case FLEXI_TYPE_INDIRECT_SINT:
        return parser_emit_sint(parser, key, cursor, user);
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_UINT:
        return parser_emit_uint(parser, key, cursor, user);
    case FLEXI_TYPE_FLOAT:
    case FLEXI_TYPE_INDIRECT_FLOAT:
        return parser_emit_flt(parser, key, cursor, user);
    case FLEXI_TYPE_KEY:
        parser->key(key, cursor->cursor, user);
        return FLEXI_OK;
    case FLEXI_TYPE_STRING:
        return parser_emit_string(parser, key, cursor, user);
    case FLEXI_TYPE_MAP:
        limits->iterables += 1;
        if (limits->iterables >= FLEXI_CONFIG_MAX_ITERABLES) {
            // Defuse FlexBuffer "bomb" inputs.
            return FLEXI_ERR_PARSELIMIT;
        }
        return parser_emit_map(parser, key, cursor, user, limits);
    case FLEXI_TYPE_VECTOR: {
        limits->iterables += 1;
        if (limits->iterables >= FLEXI_CONFIG_MAX_ITERABLES) {
            // Defuse FlexBuffer "bomb" inputs.
            return FLEXI_ERR_PARSELIMIT;
        }
        return parser_emit_vector(parser, key, cursor, user, limits);
    }
    case FLEXI_TYPE_VECTOR_SINT:
    case FLEXI_TYPE_VECTOR_UINT:
    case FLEXI_TYPE_VECTOR_FLOAT:
    case FLEXI_TYPE_VECTOR_BOOL:
        return parser_emit_typed_vector(parser, key, cursor, user);
    case FLEXI_TYPE_VECTOR_KEY:
        return parser_emit_vector_keys(parser, key, cursor, user);
    case FLEXI_TYPE_VECTOR_SINT2:
    case FLEXI_TYPE_VECTOR_UINT2:
    case FLEXI_TYPE_VECTOR_FLOAT2:
        parser->typed_vector(key, cursor->cursor, 2, cursor->type,
            cursor->width, user);
        return FLEXI_OK;
    case FLEXI_TYPE_VECTOR_SINT3:
    case FLEXI_TYPE_VECTOR_UINT3:
    case FLEXI_TYPE_VECTOR_FLOAT3:
        parser->typed_vector(key, cursor->cursor, 3, cursor->type,
            cursor->width, user);
        return FLEXI_OK;
    case FLEXI_TYPE_VECTOR_SINT4:
    case FLEXI_TYPE_VECTOR_UINT4:
    case FLEXI_TYPE_VECTOR_FLOAT4:
        parser->typed_vector(key, cursor->cursor, 4, cursor->type,
            cursor->width, user);
        return FLEXI_OK;
    case FLEXI_TYPE_BLOB:
        return parser_emit_vector_blob(parser, key, cursor, user);
    case FLEXI_TYPE_BOOL:
        parser->boolean(key, *(const bool *)(cursor->cursor), user);
        return FLEXI_OK;
    }

    ASSERT(false && "");
    return FLEXI_ERR_INTERNAL;
}

/**
 * @brief Push a value onto the stack.
 *
 * @param[in] writer Writer to operate on.
 * @return Stack value below pushed head, or NULL if no more room.
 */
static flexi_value_s *
writer_push(flexi_writer_s *writer)
{
    if (writer->head >= (flexi_stack_idx_t)COUNTOF(writer->stack)) {
        return NULL;
    }

    flexi_value_s *value = &writer->stack[writer->head];
    writer->head += 1;
    return value;
}

/**
 * @brief Peek at the n-th value from the current tail of the stack.  0 is
 *        the tail of the stack and returns a value if the stack contains
 *        any values.
 *
 * @param[in] writer Offset to use.
 * @param[in] offset Offset from the tail.
 * @return Value at stack position, or NULL if offset out of range.
 */
static flexi_value_s *
writer_peek_tail(flexi_writer_s *writer, int offset)
{
    if (offset < 0 || offset >= writer->head) {
        return NULL;
    }

    flexi_value_s *value = &writer->stack[offset];
    return value;
}

/**
 * @brief Peek at the value in the stack, starting with the n-th value in the
 *        stack (0 is head, towards tail) and then offseting by the given
 *        index (0 is start value, towards head).
 *
 * @param[in] writer Writer to use.
 * @param[in] start Stack offset to start from (towards tail).
 * @param[in] index Positive offset from the starting stack offset (towards
 *                  head).
 * @return Value at stack position, or NULL if offset is invalid
 */
static flexi_value_s *
writer_peek_idx(flexi_writer_s *writer, int start, int index)
{
    start = writer->head - start;
    if (start < 0 || start >= writer->head) {
        return NULL;
    }

    int offset = start + index;
    if (offset < 0 || offset >= writer->head) {
        return NULL;
    }

    flexi_value_s *value = &writer->stack[offset];
    return value;
}

/**
 * @brief Pop a value off the stack.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] count Number of values to pop.
 * @return True if pop was successful, or false if stack was underflowed.
 */
static bool
writer_pop(flexi_writer_s *writer, size_t count)
{
    if (writer->head < (flexi_stack_idx_t)count) {
        return false;
    }

    writer->head -= (flexi_stack_idx_t)count;
    return true;
}

/**
 * @brief Obtain the key for a map while sorting.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] start Starting index in the stack for the map.
 * @param[in] index Array-based index for the map.
 * @return Pointer to key off the stack.
 */
static flexi_value_s *
writer_get_stack_key(flexi_writer_s *writer, flexi_stack_idx_t start, int index)
{
    int v_index = start + index;
    ASSERT(v_index >= 0 && v_index < writer->head);
    flexi_value_s *v = &writer->stack[v_index];
    ASSERT(v->type == FLEXI_TYPE_KEY);
    return v;
}

/**
 * @brief Obtain the value for a map while sorting.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] start Starting index in the stack for the map.
 * @param[in] index Array-based index for the map.
 * @return Pointer to key off the stack.
 */
static flexi_value_s *
writer_get_stack_value(flexi_writer_s *writer, flexi_stack_idx_t start,
    int index)
{
    int v_index = start + index;
    ASSERT(v_index >= 0 && v_index < writer->head);
    return &writer->stack[v_index];
}

/**
 * @brief Sort the keys of a map so all keys are in strcmp order.
 *
 * @param writer[in] Writer to operate on.
 * @param len[in] Number of keys to use for map.
 */
static void
writer_sort_map_keys(flexi_writer_s *writer, size_t len)
{
    int start = writer->head - (flexi_stack_idx_t)len;
    ASSERT(start >= 0);

    // TODO: Use a faster sort for large key sets.
    for (flexi_stack_idx_t i = 1; i < (flexi_stack_idx_t)len; i++) {
        flexi_value_s *cur = writer_get_stack_key(writer, start, i);
        size_t curoff = cur->u.offset;
        const char *curkey = (const char *)writer_data_at(writer, curoff);

        int j = i;
        for (; j > 0; j--) {
            flexi_value_s *seek = writer_get_stack_key(writer, start, j - 1);
            const char *seekkey =
                (const char *)writer_data_at(writer, seek->u.offset);

            int cmp = strcmp(curkey, seekkey);
            if (cmp > 0) {
                break;
            }

            flexi_value_s *dest = writer_get_stack_key(writer, start, j);
            dest->u.offset = seek->u.offset;
        }

        if (i != j) {
            flexi_value_s *dest = writer_get_stack_key(writer, start, j);
            dest->u.offset = curoff;
        }
    }
}

/**
 * @brief Sort the values of a map so all values keys are in strcmp order.
 *
 * @param writer[in] Writer to operate on.
 * @param len[in] Number of values to use for map.
 */
static void
writer_sort_map_values(flexi_writer_s *writer, size_t len)
{
    int start = writer->head - (flexi_stack_idx_t)len;
    ASSERT(start >= 0);

    // TODO: Use a faster sort for large key sets.
    for (flexi_stack_idx_t i = 1; i < (flexi_stack_idx_t)len; i++) {
        flexi_value_s cur = *writer_get_stack_value(writer, start, i);
        const char *curkey = cur.key;

        int j = i;
        for (; j > 0; j--) {
            flexi_value_s seek = *writer_get_stack_value(writer, start, j - 1);
            const char *seekkey = seek.key;

            int cmp = strcmp(curkey, seekkey);
            if (cmp > 0) {
                break;
            }

            flexi_value_s *dest = writer_get_stack_value(writer, start, j);
            memcpy(dest, &seek, sizeof(flexi_value_s));
        }

        if (i != j) {
            flexi_value_s *dest = writer_get_stack_value(writer, start, j);
            memcpy(dest, &cur, sizeof(flexi_value_s));
        }
    }
}

/**
 * @brief Write out the values of a vector.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] len Number of values to write.
 * @param[in] stride Number of bytes between values.
 * @return FLEXI_OK Successful write.
 * @return FLEXI_ERR_BADWRITE One or more values failed to write.
 * @return FLEXI_ERR_STREAM Encountered a non-writing problem with the stream.
 */
static flexi_result_e
write_vector_values(flexi_writer_s *writer, size_t len, int stride)
{
    // Write values
    for (int i = 0; i < (int)len; i++) {
        const flexi_value_s *value = writer_peek_idx(writer, (int)len, i);
        switch (value->type) {
        case FLEXI_TYPE_SINT:
            // Write value inline.
            if (!write_sint_by_width(writer, value->u.s64, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
            break;
        case FLEXI_TYPE_UINT:
            // Write value inline.
            if (!write_uint_by_width(writer, value->u.u64, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
            break;
        case FLEXI_TYPE_FLOAT:
            // Write value inline.
            switch (value->width) {
            case 4:
                if (!write_f32(writer, value->u.f32, stride)) {
                    return FLEXI_ERR_BADWRITE;
                }
                break;
            case 8:
                if (!write_f64(writer, value->u.f64, stride)) {
                    return FLEXI_ERR_BADWRITE;
                }
                break;
            default: return FLEXI_ERR_INTERNAL;
            }
            break;
        case FLEXI_TYPE_STRING:
        case FLEXI_TYPE_INDIRECT_SINT:
        case FLEXI_TYPE_INDIRECT_UINT:
        case FLEXI_TYPE_INDIRECT_FLOAT:
        case FLEXI_TYPE_BLOB: {
            // Get the current cursor position.
            size_t current;
            if (!writer_tell(writer, &current)) {
                return FLEXI_ERR_STREAM;
            }

            // Calculate and write offset.
            size_t offset = current - value->u.offset;
            if (!write_uint_by_width(writer, offset, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
            break;
        }
        case FLEXI_TYPE_BOOL:
            // Write value inline.
            if (!write_uint_by_width(writer, value->u.u64, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
            break;
        default: return FLEXI_ERR_INTERNAL;
        }
    }
    return FLEXI_OK;
}

/**
 * @brief Write out all types of a vector.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] len Number of types.
 * @return True if write was successful.
 */
static bool
write_vector_types(flexi_writer_s *writer, size_t len)
{
    for (int i = 0; i < (int)len; i++) {
        const flexi_value_s *value = writer_peek_idx(writer, (int)len, i);
        flexi_packed_t packed =
            PACK_TYPE(value->type) | PACK_WIDTH(value->width);
        if (!writer_write(writer, &packed, sizeof(flexi_packed_t))) {
            return false;
        }
    }

    return true;
}

/******************************************************************************/

flexi_buffer_s
flexi_make_buffer(const void *buffer, size_t len)
{
    flexi_buffer_s rvo;
    rvo.data = (const char *)buffer;
    rvo.length = len;
    return rvo;
}

/******************************************************************************/

flexi_result_e
flexi_open_buffer(const flexi_buffer_s *buffer, flexi_cursor_s *cursor)
{
    if (buffer->length < 3) {
        // Shortest length we can discard without checking.
        return FLEXI_ERR_BADREAD;
    }

    // Width of root object.
    cursor->buffer = *buffer;
    cursor->cursor = buffer->data + buffer->length - 1;
    uint8_t root_bytes = *(const uint8_t *)(cursor->cursor);
    if (root_bytes == 0 || buffer->length < root_bytes + 2u) {
        return FLEXI_ERR_BADREAD;
    }

    // Obtain the packed type.
    cursor->cursor -= 1;
    flexi_packed_t packed = *(const uint8_t *)(cursor->cursor);

    // Point at the root object.
    flexi_type_e type = UNPACK_TYPE(packed);
    cursor->cursor -= root_bytes;
    if (type_is_direct(type)) {
        // No need to resolve an offset, we're done.
        cursor->type = type;
        cursor->width = root_bytes;
        return FLEXI_OK;
    }

    // We're pointing at an offset, resolve it.
    size_t offset = 0;
    if (!buffer_read_size_by_width(&cursor->buffer, cursor->cursor, root_bytes,
            &offset)) {
        return FLEXI_ERR_BADREAD;
    }

    const char *dest = NULL;
    if (!buffer_seek_back(&cursor->buffer, cursor->cursor, offset, &dest)) {
        return FLEXI_ERR_BADREAD;
    }

    cursor->cursor = dest;
    cursor->type = type;
    cursor->width = WIDTH_TO_BYTES(UNPACK_WIDTH(packed));
    return FLEXI_OK;
}

/******************************************************************************/

flexi_type_e
flexi_cursor_type(const flexi_cursor_s *cursor)
{
    return cursor->type;
}

/******************************************************************************/

int
flexi_cursor_width(const flexi_cursor_s *cursor)
{
    return cursor->width;
}

/******************************************************************************/

size_t
flexi_cursor_length(const flexi_cursor_s *cursor)
{
    switch (cursor->type) {
    case FLEXI_TYPE_STRING:
    case FLEXI_TYPE_MAP:
    case FLEXI_TYPE_VECTOR:
    case FLEXI_TYPE_VECTOR_SINT:
    case FLEXI_TYPE_VECTOR_UINT:
    case FLEXI_TYPE_VECTOR_FLOAT:
    case FLEXI_TYPE_BLOB:
    case FLEXI_TYPE_VECTOR_BOOL: {
        size_t len;
        if (!cursor_get_len(cursor, &len)) {
            return 0;
        }
        return len;
    }
    case FLEXI_TYPE_VECTOR_SINT2:
    case FLEXI_TYPE_VECTOR_UINT2:
    case FLEXI_TYPE_VECTOR_FLOAT2: return 2;
    case FLEXI_TYPE_VECTOR_SINT3:
    case FLEXI_TYPE_VECTOR_UINT3:
    case FLEXI_TYPE_VECTOR_FLOAT3: return 3;
    case FLEXI_TYPE_VECTOR_SINT4:
    case FLEXI_TYPE_VECTOR_UINT4:
    case FLEXI_TYPE_VECTOR_FLOAT4: return 4;
    default: return 0;
    }
}

/******************************************************************************/

flexi_result_e
flexi_cursor_sint(const flexi_cursor_s *cursor, int64_t *val)
{
    if (type_is_sint(cursor->type)) {
        int64_t v;
        if (!buffer_read_sint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }

        *val = v;
        return FLEXI_OK;
    } else if (type_is_uint(cursor->type)) {
        uint64_t v;
        if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }
        if (v > INT64_MAX) {
            *val = INT64_MAX;
            return FLEXI_ERR_RANGE;
        }

        *val = (int64_t)v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 4) {
        float v;
        if (!buffer_read_f32(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }
        if (v > INT64_MAX) {
            *val = INT64_MAX;
            return FLEXI_ERR_RANGE;
        }
        if (v < INT64_MIN) {
            *val = INT64_MIN;
            return FLEXI_ERR_RANGE;
        }

        *val = (int64_t)v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 8) {
        double v;
        if (!buffer_read_f64(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }
        if (v > INT64_MAX) {
            *val = INT64_MAX;
            return FLEXI_ERR_RANGE;
        }
        if (v < INT64_MIN) {
            *val = INT64_MIN;
            return FLEXI_ERR_RANGE;
        }

        *val = (int64_t)v;
        return FLEXI_OK;
    } else if (cursor->type == FLEXI_TYPE_BOOL && cursor->width == 1) {
        *val = *(const bool *)(cursor->cursor);
        return FLEXI_OK;
    }

    *val = 0;
    return FLEXI_ERR_BADTYPE;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_uint(const flexi_cursor_s *cursor, uint64_t *val)
{
    if (type_is_sint(cursor->type)) {
        int64_t v;
        if (!buffer_read_sint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }
        if (v < 0) {
            *val = 0;
            return FLEXI_ERR_RANGE;
        }

        *val = (uint64_t)v;
        return FLEXI_OK;
    } else if (type_is_uint(cursor->type)) {
        uint64_t v;
        if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }

        *val = v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 4) {
        float v;
        if (!buffer_read_f32(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }

        *val = (uint64_t)v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 8) {
        double v;
        if (!buffer_read_f64(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0;
            return FLEXI_ERR_BADREAD;
        }

        *val = (uint64_t)v;
        return FLEXI_OK;
    } else if (cursor->type == FLEXI_TYPE_BOOL && cursor->width == 1) {
        *val = *(const bool *)(cursor->cursor);
        return FLEXI_OK;
    }

    *val = 0;
    return FLEXI_ERR_BADTYPE;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_f32(const flexi_cursor_s *cursor, float *val)
{
    if (type_is_sint(cursor->type)) {
        int64_t v;
        if (!buffer_read_sint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0f;
            return FLEXI_ERR_BADREAD;
        }

        *val = (float)v;
        return FLEXI_OK;
    } else if (type_is_uint(cursor->type)) {
        uint64_t v;
        if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0f;
            return FLEXI_ERR_BADREAD;
        }

        *val = (float)v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 4) {
        float v;
        if (!buffer_read_f32(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0f;
            return FLEXI_ERR_BADREAD;
        }

        *val = v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 8) {
        double v;
        if (!buffer_read_f64(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0f;
            return FLEXI_ERR_BADREAD;
        }

        *val = (float)v;
        return FLEXI_OK;
    } else if (cursor->type == FLEXI_TYPE_BOOL && cursor->width == 1) {
        *val = (float)*(const bool *)(cursor->cursor);
        return FLEXI_OK;
    }

    *val = 0.0f;
    return FLEXI_ERR_BADTYPE;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_f64(const flexi_cursor_s *cursor, double *val)
{
    if (type_is_sint(cursor->type)) {
        int64_t v;
        if (!buffer_read_sint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        *val = (double)v;
        return FLEXI_OK;
    } else if (type_is_uint(cursor->type)) {
        uint64_t v;
        if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        *val = (double)v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 4) {
        float v;
        if (!buffer_read_f32(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        *val = v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 8) {
        double v;
        if (!buffer_read_f64(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        *val = v;
        return FLEXI_OK;
    } else if (cursor->type == FLEXI_TYPE_BOOL && cursor->width == 1) {
        *val = (double)*(const bool *)(cursor->cursor);
        return FLEXI_OK;
    }

    *val = 0.0;
    return FLEXI_ERR_BADTYPE;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_key(const flexi_cursor_s *cursor, const char **str)
{
    switch (cursor->type) {
    case FLEXI_TYPE_KEY: break;
    case FLEXI_TYPE_STRING: break;
    default: *str = ""; return FLEXI_ERR_BADTYPE;
    }

    *str = cursor->cursor;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_string(const flexi_cursor_s *cursor, const char **str)
{
    switch (cursor->type) {
    case FLEXI_TYPE_KEY: break;
    case FLEXI_TYPE_STRING: break;
    default: *str = ""; return FLEXI_ERR_BADTYPE;
    }

    *str = cursor->cursor;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index,
    const char **str)
{
    if (cursor->type != FLEXI_TYPE_MAP) {
        *str = "";
        return FLEXI_ERR_BADTYPE;
    }

    if (!cursor_map_key_at_index(cursor, index, str)) {
        *str = "";
        return FLEXI_ERR_INTERNAL;
    }

    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_seek_map_key(const flexi_cursor_s *cursor, const char *key,
    flexi_cursor_s *dest)
{
    if (cursor->type != FLEXI_TYPE_MAP) {
        return FLEXI_ERR_BADTYPE;
    }

    size_t len = 0;
    if (!cursor_get_len(cursor, &len)) {
        return FLEXI_ERR_INTERNAL;
    }

    if (len <= 16) {
        return cursor_seek_map_key_linear(cursor, len, key, dest);
    } else {
        return cursor_seek_map_key_bsearch(cursor, len, key, dest);
    }
}

/******************************************************************************/

flexi_result_e
flexi_cursor_vector_types(const flexi_cursor_s *cursor,
    const flexi_packed_t **packed)
{
    if (cursor->type != FLEXI_TYPE_VECTOR && cursor->type != FLEXI_TYPE_MAP) {
        return FLEXI_ERR_BADTYPE;
    }

    return cursor_vector_types(cursor, packed) ? FLEXI_OK : FLEXI_ERR_INTERNAL;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index,
    flexi_cursor_s *dest)
{
    if (cursor->type != FLEXI_TYPE_VECTOR && cursor->type != FLEXI_TYPE_MAP) {
        return FLEXI_ERR_BADTYPE;
    }

    return cursor_seek_vector_index(cursor, index, dest) ? FLEXI_OK
                                                         : FLEXI_ERR_INTERNAL;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_typed_vector_data(const flexi_cursor_s *cursor, const void **data)
{
    if (cursor->type == FLEXI_TYPE_VECTOR || !type_is_vector(cursor->type)) {
        *data = "";
        return FLEXI_ERR_BADTYPE;
    }

    *data = cursor->cursor;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_blob(const flexi_cursor_s *cursor, const uint8_t **blob)
{
    if (cursor->type != FLEXI_TYPE_BLOB) {
        *blob = (const uint8_t *)"";
        return FLEXI_ERR_BADTYPE;
    }

    *blob = (const uint8_t *)cursor->cursor;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_bool(const flexi_cursor_s *cursor, bool *val)
{
    if (type_is_anyint(cursor->type)) {
        uint64_t v;
        if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = false;
            return FLEXI_ERR_BADREAD;
        }

        *val = (bool)v;
        return FLEXI_OK;
    } else if (cursor->type == FLEXI_TYPE_FLOAT &&
               cursor->width == sizeof(float)) {
        float v;
        if (!buffer_read_f32(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = false;
            return FLEXI_ERR_BADREAD;
        }

        *val = (bool)v;
        return FLEXI_OK;
    } else if (cursor->type == FLEXI_TYPE_FLOAT &&
               cursor->width == sizeof(double)) {
        double v;
        if (!buffer_read_f64(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = false;
            return FLEXI_ERR_BADREAD;
        }

        *val = (bool)v;
        return FLEXI_OK;
    } else if (cursor->type == FLEXI_TYPE_BOOL) {
        *val = *(const bool *)(cursor->cursor);
        return FLEXI_OK;
    }

    *val = false;
    return FLEXI_ERR_BADTYPE;
}

/******************************************************************************/

flexi_result_e
flexi_parse_cursor(const flexi_parser_s *parser, const flexi_cursor_s *cursor,
    void *user)
{
    parse_limits_s limits = {0};
    return parse_cursor(parser, NULL, cursor, user, &limits);
}

/******************************************************************************/

flexi_result_e
flexi_write_null_keyed(flexi_writer_s *writer, const char *k)
{
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.u64 = 0;
    stack->key = k;
    stack->type = FLEXI_TYPE_NULL;
    stack->width = 1;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_sint_keyed(flexi_writer_s *writer, const char *k, int64_t v)
{
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.u64 = v;
    stack->key = k;
    stack->type = FLEXI_TYPE_SINT;
    stack->width = SINT_WIDTH(v);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_uint_keyed(flexi_writer_s *writer, const char *k, uint64_t v)
{
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.u64 = v;
    stack->key = k;
    stack->type = FLEXI_TYPE_UINT;
    stack->width = UINT_WIDTH(v);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_f32_keyed(flexi_writer_s *writer, const char *k, float v)
{
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.f32 = v;
    stack->key = k;
    stack->type = FLEXI_TYPE_FLOAT;
    stack->width = sizeof(float);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_f64_keyed(flexi_writer_s *writer, const char *k, double v)
{
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.f64 = v;
    stack->key = k;
    stack->type = FLEXI_TYPE_FLOAT;
    stack->width = sizeof(double);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_string_keyed(flexi_writer_s *writer, const char *k, const char *str)
{
    // Write the string length to stream.
    size_t len = strlen(str);
    int width = UINT_WIDTH(len);
    if (!write_uint_by_width(writer, len, width)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Keep track of string starting position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return FLEXI_ERR_STREAM;
    }

    // Write the string, plus the trailing '\0'.
    if (!writer_write(writer, str, len + 1)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_STRING;
    stack->width = width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_key_keyed(flexi_writer_s *writer, const char *k, const char *str)
{
    // Keep track of string starting position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return FLEXI_ERR_STREAM;
    }

    // Write the string, plus the trailing '\0'.
    size_t len = strlen(str);
    if (!writer_write(writer, str, len + 1)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_KEY;
    stack->width = 0;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_sint_keyed(flexi_writer_s *writer, const char *k,
    int64_t v)
{
    int width = SINT_WIDTH(v);

    // Align to the nearest multiple.
    size_t offset;
    if (!write_padding(writer, 0, width, &offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_sint_by_width(writer, v, width)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_INDIRECT_SINT;
    stack->width = width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_uint_keyed(flexi_writer_s *writer, const char *k,
    uint64_t v)
{
    int width = UINT_WIDTH(v);

    // Align to the nearest multiple.
    size_t offset;
    if (!write_padding(writer, 0, width, &offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_uint_by_width(writer, v, width)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_INDIRECT_UINT;
    stack->width = width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_f32_keyed(flexi_writer_s *writer, const char *k, float v)
{
    // Align to the nearest multiple.
    size_t offset;
    if (!write_padding(writer, 0, sizeof(float), &offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_f32(writer, v, sizeof(float))) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_INDIRECT_FLOAT;
    stack->width = sizeof(float);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_f64_keyed(flexi_writer_s *writer, const char *k, double v)
{
    // Keep track of the value's position.
    size_t offset;
    if (!write_padding(writer, 0, sizeof(double), &offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_f64(writer, v, sizeof(double))) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_INDIRECT_FLOAT;
    stack->width = sizeof(double);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_map_keys(flexi_writer_s *writer, size_t len, flexi_width_e stride,
    flexi_stack_idx_t *keyset)
{
    // Find the start of the keys on the stack.
    int start = writer->head - (flexi_stack_idx_t)len;
    if (start < 0) {
        return FLEXI_ERR_BADSTACK;
    }

    // All keys must be of key type.  Avoids redundant checks later.
    for (int i = start; i < writer->head; i++) {
        if (writer->stack[i].type != FLEXI_TYPE_KEY) {
            return FLEXI_ERR_NOTKEYS;
        }
    }

    // Sort the keys by key name.
    writer_sort_map_keys(writer, len);

    int stride_bytes = WIDTH_TO_BYTES(stride);

    // Write length
    if (!write_uint_by_width(writer, len, stride_bytes)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Keep track of the base location of the keys.
    size_t keys_offset;
    if (!writer_tell(writer, &keys_offset)) {
        return FLEXI_ERR_STREAM;
    }

    // Write out key offsets.
    for (int i = start; i < writer->head; i++) {
        flexi_value_s *value = &writer->stack[i];
        ASSERT(value->type == FLEXI_TYPE_KEY);

        // TODO: We can pre-calculate this.
        size_t current;
        if (!writer_tell(writer, &current)) {
            return FLEXI_ERR_STREAM;
        }

        size_t offset = current - value->u.offset;
        if (!write_uint_by_width(writer, offset, stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }
    }

    bool ok = writer_pop(writer, len);
    if (!ok) {
        return FLEXI_ERR_BADSTACK;
    }

    *keyset = writer->head;
    flexi_value_s *stack = writer_push(writer);
    ASSERT(stack && "Stack is in invalid state.");

    stack->type = FLEXI_TYPE_VECTOR_KEY;
    stack->u.offset = keys_offset;
    stack->width = stride_bytes;
    stack->key = NULL;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_map_keyed(flexi_writer_s *writer, const char *k,
    flexi_stack_idx_t keyset, size_t len, flexi_width_e stride)
{
    // First seek out the keys.
    flexi_value_s *keys_value = writer_peek_tail(writer, keyset);
    if (keys_value == NULL || keys_value->type != FLEXI_TYPE_VECTOR_KEY) {
        return FLEXI_ERR_BADTYPE;
    }

    // Sort the values based on their keys.
    writer_sort_map_values(writer, len);

    int stride_bytes = WIDTH_TO_BYTES(stride);

    // Offset to aligned keys vector.
    size_t current;
    if (!write_padding(writer, 0, stride_bytes, &current)) {
        return FLEXI_ERR_BADWRITE;
    }

    size_t offset = current - keys_value->u.offset;
    if (!write_uint_by_width(writer, offset, stride_bytes)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Byte width of key vector.
    if (!write_sint_by_width(writer, keys_value->width, stride_bytes)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Now we're to the values - write length.
    if (!write_uint_by_width(writer, len, stride_bytes)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Keep track of the base location of the vector.
    size_t values_offset;
    if (!writer_tell(writer, &values_offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write values.
    flexi_result_e res = write_vector_values(writer, len, stride_bytes);
    if (FLEXI_ERROR(res)) {
        return res;
    }

    // Write types.
    if (!write_vector_types(writer, len)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Pop the values.
    bool ok = writer_pop(writer, len);
    if (!ok) {
        return FLEXI_ERR_BADSTACK;
    }

    // Push the completed map.
    flexi_value_s *stack = writer_push(writer);
    ASSERT(stack && "Stack is in invalid state.");

    stack->u.offset = values_offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_MAP;
    stack->width = stride_bytes;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_vector_keyed(flexi_writer_s *writer, const char *k, size_t len,
    flexi_width_e stride)
{
    int stride_bytes = WIDTH_TO_BYTES(stride);

    // Align future writes to the nearest multiple.
    size_t offset;
    if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write length
    if (!write_uint_by_width(writer, len, stride_bytes)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write values.
    flexi_result_e res = write_vector_values(writer, len, stride_bytes);
    if (FLEXI_ERROR(res)) {
        return res;
    }

    // Write types.
    if (!write_vector_types(writer, len)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Pop values.
    bool ok = writer_pop(writer, len);
    if (!ok) {
        return FLEXI_ERR_BADSTACK;
    }

    // Push vector.
    flexi_value_s *stack = writer_push(writer);
    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_VECTOR;
    stack->width = stride_bytes;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_sint_keyed(flexi_writer_s *writer, const char *k,
    const void *ptr, flexi_width_e stride, size_t len)
{
    int stride_bytes = WIDTH_TO_BYTES(stride);
    ASSERT(stride_bytes >= 1 && stride_bytes <= 8);

    if (len >= 2 && len <= 4) {
        // Keep track of the aligned base location of the vector.
        size_t offset;
        if (!write_padding(writer, 0, stride_bytes, &offset)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write values.
        if (!writer_write(writer, ptr, len * stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        flexi_value_s *stack = writer_push(writer);
        stack->u.offset = offset;
        stack->key = k;
        stack->type = len == 2   ? FLEXI_TYPE_VECTOR_SINT2
                      : len == 3 ? FLEXI_TYPE_VECTOR_SINT3
                                 : FLEXI_TYPE_VECTOR_SINT4;
        stack->width = stride_bytes;
        return FLEXI_OK;
    } else {
        // Align future writes to the nearest multiple.
        size_t offset;
        if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Length prefix.
        if (!write_uint_by_width(writer, len, stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write values.
        if (!writer_write(writer, ptr, len * stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        flexi_value_s *stack = writer_push(writer);
        stack->u.offset = offset;
        stack->key = k;
        stack->type = FLEXI_TYPE_VECTOR_SINT;
        stack->width = stride_bytes;
        return FLEXI_OK;
    }
}

/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_uint_keyed(flexi_writer_s *writer, const char *k,
    const void *ptr, flexi_width_e stride, size_t len)
{
    int stride_bytes = WIDTH_TO_BYTES(stride);
    ASSERT(stride_bytes >= 1 && stride_bytes <= 8);

    if (len >= 2 && len <= 4) {
        // Keep track of the aligned base location of the vector.
        size_t offset;
        if (!write_padding(writer, 0, stride_bytes, &offset)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write values.
        if (!writer_write(writer, ptr, len * stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        flexi_value_s *stack = writer_push(writer);
        stack->u.offset = offset;
        stack->key = k;
        stack->type = len == 2   ? FLEXI_TYPE_VECTOR_UINT2
                      : len == 3 ? FLEXI_TYPE_VECTOR_UINT3
                                 : FLEXI_TYPE_VECTOR_UINT4;
        stack->width = stride_bytes;
        return FLEXI_OK;
    } else {
        // Align future writes to the nearest multiple.
        size_t offset;
        if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Length prefix.
        if (!write_uint_by_width(writer, len, stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write values.
        if (!writer_write(writer, ptr, len * stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        flexi_value_s *stack = writer_push(writer);
        stack->u.offset = offset;
        stack->key = k;
        stack->type = FLEXI_TYPE_VECTOR_UINT;
        stack->width = stride_bytes;
        return FLEXI_OK;
    }
}
/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_flt_keyed(flexi_writer_s *writer, const char *k,
    const void *ptr, flexi_width_e stride, size_t len)
{
    int stride_bytes = WIDTH_TO_BYTES(stride);
    if (stride_bytes != 4 && stride_bytes != 8) {
        return FLEXI_ERR_BADTYPE;
    }

    if (len >= 2 && len <= 4) {
        // Keep track of the aligned base location of the vector.
        size_t offset;
        if (!write_padding(writer, 0, stride_bytes, &offset)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write values.
        if (!writer_write(writer, ptr, len * stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        flexi_value_s *stack = writer_push(writer);
        stack->u.offset = offset;
        stack->key = k;
        stack->type = len == 2   ? FLEXI_TYPE_VECTOR_FLOAT2
                      : len == 3 ? FLEXI_TYPE_VECTOR_FLOAT3
                                 : FLEXI_TYPE_VECTOR_FLOAT4;
        stack->width = stride_bytes;
        return FLEXI_OK;
    } else {
        // Align future writes to the nearest multiple.
        size_t offset;
        if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Length prefix.
        if (!write_uint_by_width(writer, len, stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write values.
        if (!writer_write(writer, ptr, len * stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }

        flexi_value_s *stack = writer_push(writer);
        stack->u.offset = offset;
        stack->key = k;
        stack->type = FLEXI_TYPE_VECTOR_FLOAT;
        stack->width = stride_bytes;
        return FLEXI_OK;
    }
}

/******************************************************************************/

flexi_result_e
flexi_write_blob_keyed(flexi_writer_s *writer, const char *k, const void *ptr,
    size_t len, int align)
{
    int len_width = UINT_WIDTH(len);

    // Pad the blob to the alignment value.
    size_t offset;
    if (!write_padding(writer, len_width, align, &offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write the blob length to stream.
    if (!write_uint_by_width(writer, len, len_width)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write the blob.
    if (!writer_write(writer, ptr, len)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_BLOB;
    stack->width = len_width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_bool_keyed(flexi_writer_s *writer, const char *k, bool v)
{
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.u64 = v;
    stack->key = k;
    stack->type = FLEXI_TYPE_BOOL;
    stack->width = 1;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_bool_keyed(flexi_writer_s *writer, const char *k,
    const bool *ptr, size_t len)
{
    // Length prefix.
    if (!write_uint_by_width(writer, len, sizeof(bool))) {
        return FLEXI_ERR_BADWRITE;
    }

    // Keep track of the base location of the vector.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return FLEXI_ERR_STREAM;
    }

    // Write values.
    if (!writer_write(writer, ptr, len * sizeof(bool))) {
        return FLEXI_ERR_BADWRITE;
    }

    flexi_value_s *stack = writer_push(writer);
    stack->u.offset = offset;
    stack->key = k;
    stack->type = FLEXI_TYPE_VECTOR_FLOAT;
    stack->width = sizeof(bool);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_finalize(flexi_writer_s *writer)
{
    flexi_value_s *root = writer_peek_idx(writer, 1, 0);
    if (root == NULL) {
        // We have nothing on the stack to write.
        return FLEXI_ERR_BADSTACK;
    }

    if (type_is_direct(root->type)) {
        switch (root->type) {
        case FLEXI_TYPE_NULL: {
            // Write out the null root type.
            const uint8_t buffer[3] = {0x00, 0x00, 0x01};
            if (!writer_write(writer, buffer, COUNTOF(buffer))) {
                return FLEXI_ERR_BADWRITE;
            }
            return FLEXI_OK;
        }
        case FLEXI_TYPE_SINT: {
            // Write the number.
            if (!write_sint_by_width(writer, root->u.s64, root->width)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Write the type.
            flexi_packed_t type =
                PACK_TYPE(root->type) | PACK_WIDTH(root->width);
            if (!writer_write(writer, &type, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Write the width.
            if (!writer_write(writer, &root->width, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            return FLEXI_OK;
        }
        case FLEXI_TYPE_UINT: {
            // Write the number.
            if (!write_uint_by_width(writer, root->u.u64, root->width)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Write the type.
            flexi_packed_t type =
                PACK_TYPE(root->type) | PACK_WIDTH(root->width);
            if (!writer_write(writer, &type, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Write the width.
            if (!writer_write(writer, &root->width, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            return FLEXI_OK;
        }
        case FLEXI_TYPE_FLOAT: {
            // Write the number.
            switch (root->width) {
            case 4:
                if (!write_f32(writer, root->u.f32, root->width)) {
                    return FLEXI_ERR_BADWRITE;
                }
                break;
            case 8:
                if (!write_f64(writer, root->u.f64, root->width)) {
                    return FLEXI_ERR_BADWRITE;
                }
                break;
            default: return FLEXI_ERR_INTERNAL;
            }

            // Write the type.
            flexi_packed_t type =
                PACK_TYPE(root->type) | PACK_WIDTH(root->width);
            if (!writer_write(writer, &type, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Write the width.
            if (!writer_write(writer, &root->width, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            return FLEXI_OK;
        }
        case FLEXI_TYPE_BOOL: {
            // Write the boolean.
            bool value = root->u.u64 ? 1 : 0;
            if (!writer_write(writer, &value, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Write the type.
            flexi_packed_t type = PACK_TYPE(root->type) | FLEXI_WIDTH_1B;
            if (!writer_write(writer, &type, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Write the width.
            if (!writer_write(writer, &root->width, 1)) {
                return FLEXI_ERR_BADWRITE;
            }

            return FLEXI_OK;
        }
        default:
            ASSERT(false && "Unhandled direct type.");
            return FLEXI_ERR_INTERNAL;
        }
    } else if (type_is_indirect(root->type)) {
        // Get the current location.
        size_t current;
        if (!writer_tell(writer, &current)) {
            return FLEXI_ERR_STREAM;
        }

        // Create the offset and figure out its witdh.
        size_t offset = current - root->u.offset;
        int width = UINT_WIDTH(offset);
        if (!write_uint_by_width(writer, offset, width)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write the type that the offset is pointing to.
        flexi_packed_t type = PACK_TYPE(root->type) | PACK_WIDTH(root->width);
        if (!write_uint_by_width(writer, type, 1)) {
            return FLEXI_ERR_BADWRITE;
        }

        // Write the width of the offset.
        if (!write_sint_by_width(writer, width, 1)) {
            return FLEXI_ERR_BADWRITE;
        }

        bool ok = writer_pop(writer, 1);
        ASSERT(ok && "Stack is empty when it shouldn't be.");
        (void)ok;
        return FLEXI_OK;
    }

    ASSERT(false && "Invalid type found on the stack.");
    return FLEXI_ERR_INTERNAL;
}
