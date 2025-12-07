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

#ifndef FLEXI_CONFIG_SEEK_MAP_KEY_LINEAR_MAX
/**
 * @brief The maximum length of a map where keys are looked up linearly.
 *        Above this count, keys are found via binary search.
 *
 * @details With a small enough map, it is actually faster to do a linear
 *          scan of keys than to do a binary search.  This configuration
 *          value tunes the cut-off length.
 */
#define FLEXI_CONFIG_SEEK_MAP_KEY_LINEAR_MAX (16)
#endif

/******************************************************************************/

#ifndef NDEBUG
#include <stdlib.h>

/**
 * @brief Force a crash if expression evaluates to false.
 *
 * @details Asserts should only be used to validate preconditions of calling
 *          a particular internal function.  Problems found in the middle
 *          of a function should return an error.
 */
#define ASSERT(ex) ((ex) ? (void)0 : (abort(), (void)0))
#else
#define ASSERT(ex) (void)0
#endif

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
    ((v) <= UINT8_MAX ? 1 : (v) <= UINT16_MAX ? 2 : (v) <= UINT32_MAX ? 4 : 8)

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

/******************************************************************************/

static flexi_result_e
parse_cursor(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user, parse_limits_s *limits);

/******************************************************************************/

const flexi_type_e FLEXI_TYPE_INVALID = (flexi_type_e)-1;

static flexi_packed_t g_empty_packed = PACK_TYPE(FLEXI_TYPE_NULL);
static uint64_t g_empty_typed_vector = 0;
static uint8_t g_empty_blob = 0;

/******************************************************************************/

/**
 * @brief Returns true if the value has a single bit set, making it a
 *        power-of-two.
 */
static bool
has_single_bit(uintmax_t x)
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
 * @brief Returns true if the type has a length prefix.
 */
static bool
type_has_length_prefix(flexi_type_e type)
{
    switch (type) {
    case FLEXI_TYPE_STRING:
    case FLEXI_TYPE_MAP:
    case FLEXI_TYPE_VECTOR:
    case FLEXI_TYPE_VECTOR_SINT:
    case FLEXI_TYPE_VECTOR_UINT:
    case FLEXI_TYPE_VECTOR_FLOAT:
    case FLEXI_TYPE_VECTOR_KEY:
    case FLEXI_TYPE_BLOB:
    case FLEXI_TYPE_VECTOR_BOOL: return true;
    default: return false;
    }
}

/**
 * @brief Return true if the type is a map or untyped vector, which contain
 *        a length prefix and trailing types.
 */
static bool
type_is_map_or_untyped_vector(flexi_type_e type)
{
    return type == FLEXI_TYPE_MAP || type == FLEXI_TYPE_VECTOR;
}

/**
 * @brief Return true if the type is a typed vector.
 */
static bool
type_is_typed_vector(flexi_type_e type)
{
    if (type >= FLEXI_TYPE_VECTOR_SINT && type <= FLEXI_TYPE_VECTOR_FLOAT4) {
        return true;
    } else if (type == FLEXI_TYPE_VECTOR_BOOL) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type is a typed vector of fixed length.
 */
static bool
type_is_typed_vector_fixed(flexi_type_e type)
{
    return type >= FLEXI_TYPE_VECTOR_SINT2 && type <= FLEXI_TYPE_VECTOR_FLOAT4;
}

/**
 * @brief Returns true if read would be valid for the given source and width.
 */
static bool
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
buffer_seek_back(const flexi_buffer_s *buffer, const char *src,
    flexi_ssize_t offset, const char **dest)
{
    if (offset == 0) {
        // An offset of 0 means someone is being sneaky.
        return false;
    }

    flexi_ssize_t max_offset = src - buffer->data;
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
 * @param[in] src Pointer to data to read from.
 * @param[in] width Width of data to read.
 * @param[out] dst Output value.
 * @return True if read was successful, false if size would be too large
 *         for the given size type.
 */
static bool
buffer_read_size_by_width_unsafe(const char *src, int width, flexi_ssize_t *dst)
{
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
    default: {
        ASSERT(false);
        return false;
    }
    }
}

/******************************************************************************/

static bool
buffer_read_size_by_width(const flexi_buffer_s *buffer, const char *src,
    int width, flexi_ssize_t *dst)
{
    if (!read_is_valid(buffer, src, width)) {
        return false;
    }

    return buffer_read_size_by_width_unsafe(src, width, dst);
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
    default: return false;
    }
}

/**
 * @brief Read an unsigned integer from the passed buffer.
 *
 * @note Values of width < 8 bytes will be promoted to uint64_t.
 *
 * @pre Width must be a valid width in bytes.
 *
 * @param[in] src Pointer to data to read from.
 * @param[in] width Width of data to read.
 * @return Output value.
 */
static uint64_t
buffer_read_uint_unsafe(const char *src, int width)
{
    switch (width) {
    case 1: {
        uint8_t v;
        memcpy(&v, src, 1);
        return v;
    }
    case 2: {
        uint16_t v;
        memcpy(&v, src, 2);
        return v;
    }
    case 4: {
        uint32_t v;
        memcpy(&v, src, 4);
        return v;
    }
    case 8: {
        uint64_t v;
        memcpy(&v, src, 8);
        return v;
    }
    default: {
        ASSERT(false);
        return 0;
    }
    }
}

/******************************************************************************/

static bool
buffer_read_uint(const flexi_buffer_s *buffer, const char *src, int width,
    uint64_t *dst)
{
    if (!read_is_valid(buffer, src, width)) {
        return false;
    }

    *dst = buffer_read_uint_unsafe(src, width);
    return true;
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
    default: return false;
    }
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
    default: return false;
    }
}

/**
 * @brief Wrapper for flexi_stack_s at function call.
 */
static flexi_value_s *
stack_at(const flexi_stack_s *stack, flexi_ssize_t offset)
{
    return stack->at(offset, stack->user);
}

/**
 * @brief Wrapper for flexi_stack_s count function call.
 */
static flexi_ssize_t
stack_count(const flexi_stack_s *stack)
{
    return stack->count(stack->user);
}

/**
 * @brief Wrapper for flexi_stack_s push function call.
 */
static flexi_value_s *
stack_push(flexi_stack_s *stack)
{
    return stack->push(stack->user);
}

/**
 * @brief Wrapper for flexi_stack_s pop function call.
 */
static flexi_ssize_t
stack_pop(flexi_stack_s *stack, flexi_ssize_t count)
{
    return stack->pop(count, stack->user);
}

/**
 * @brief Swap the top two values on the stack.
 */
static bool
stack_swap(flexi_stack_s *stack)
{
    flexi_ssize_t count = stack_count(stack);
    if (count < 2) {
        return false;
    }

    flexi_value_s *hi = stack_at(stack, count - 1);
    flexi_value_s *lo = stack_at(stack, count - 2);
    flexi_value_s tmp = *hi;
    *hi = *lo;
    *lo = tmp;
    return true;
}

/**
 * @brief Take the item off the top of the stack and move it down n places.
 */
static bool
stack_roll_down(flexi_stack_s *stack, flexi_ssize_t dest)
{
    flexi_ssize_t count = stack_count(stack);
    if (dest < 0 || dest >= count) {
        return false;
    }

    // Get the top item and save a copy.
    flexi_value_s *top = stack_at(stack, count - 1);
    if (top == NULL) {
        return false;
    }
    flexi_value_s tmp = *top;

    // Bubble up other items to the top of the stack.
    for (flexi_ssize_t i = count - 1; i > dest; i--) {
        flexi_value_s *src = stack_at(stack, i - 1);
        flexi_value_s *dst = stack_at(stack, i);
        *dst = *src;
    }

    // Place the saved top item at its new position
    flexi_value_s *dest_value = stack_at(stack, dest);
    *dest_value = tmp;
    return true;
}

/**
 * @brief Wrapper for flexi_ostream_s write function call.
 */
static bool
ostream_write(flexi_ostream_s *ostream, const void *ptr, flexi_ssize_t len)
{
    return ostream->write(ptr, len, ostream->user);
}

/**
 * @brief Wrapper for flexi_ostream_s data_at function call.
 */
static const void *
ostream_data_at(flexi_ostream_s *ostream, flexi_ssize_t offset)
{
    return ostream->data_at(offset, ostream->user);
}

/**
 * @brief Wrapper for flexi_ostream_s tell function call.
 */
static bool
ostream_tell(flexi_ostream_s *ostream, flexi_ssize_t *offset)
{
    return ostream->tell(offset, ostream->user);
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
write_padding(flexi_writer_s *writer, int prefix, int width,
    flexi_ssize_t *offset)
{
    if (!has_single_bit((uintmax_t)width)) {
        return false;
    }

    flexi_ssize_t src_offset;
    if (!ostream_tell(&writer->ostream, &src_offset)) {
        return false;
    }

    // Round offset to nearest multiple.
    uint64_t dst_offset = round_to_pow2_mul64(src_offset, (uint64_t)width);
    flexi_ssize_t padding_len = (flexi_ssize_t)dst_offset - src_offset;

    // Loop, writing 8 bytes at a time until we're done.
    static const char s_padding[8] = {0};
    for (; padding_len > 0; padding_len -= 8) {
        flexi_ssize_t write_len = MIN(padding_len, 8);
        if (!ostream_write(&writer->ostream, s_padding, write_len)) {
            return false;
        }
    }

    *offset = (flexi_ssize_t)prefix + (flexi_ssize_t)dst_offset;
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
    switch (width) {
    case 1: {
        if (v > INT8_MAX || v < INT8_MIN) {
            return false;
        }

        int8_t vv = (int8_t)v;
        return ostream_write(&writer->ostream, &vv, sizeof(int8_t));
    }
    case 2: {
        if (v > INT16_MAX || v < INT16_MIN) {
            return false;
        }

        int16_t vv = (int16_t)v;
        return ostream_write(&writer->ostream, &vv, sizeof(int16_t));
    }
    case 4: {
        if (v > INT32_MAX || v < INT32_MIN) {
            return false;
        }

        int32_t vv = (int32_t)v;
        return ostream_write(&writer->ostream, &vv, sizeof(int32_t));
    }
    case 8: return ostream_write(&writer->ostream, &v, sizeof(int64_t));
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
    switch (width) {
    case 1: {
        if (v > UINT8_MAX) {
            return false;
        }

        uint8_t vv = (uint8_t)v;
        return ostream_write(&writer->ostream, &vv, sizeof(uint8_t));
    }
    case 2: {
        if (v > UINT16_MAX) {
            return false;
        }

        uint16_t vv = (uint16_t)v;
        return ostream_write(&writer->ostream, &vv, sizeof(uint16_t));
    }
    case 4: {
        if (v > UINT32_MAX) {
            return false;
        }

        uint32_t vv = (uint32_t)v;
        return ostream_write(&writer->ostream, &vv, sizeof(uint32_t));
    }
    case 8: return ostream_write(&writer->ostream, &v, sizeof(uint64_t));
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
    switch (width) {
    case 4: return ostream_write(&writer->ostream, &v, sizeof(float));
    case 8: {
        double vv = v;
        return ostream_write(&writer->ostream, &vv, sizeof(double));
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
    switch (width) {
    case 4: {
        float vv = (float)v;
        return ostream_write(&writer->ostream, &vv, sizeof(float));
    }
    case 8: return ostream_write(&writer->ostream, &v, sizeof(double));
    }
    return false;
}

/**
 * @brief Check if a cursor is in an error state.
 */
static bool
cursor_is_error(const flexi_cursor_s *cursor)
{
    return cursor->type == FLEXI_TYPE_INVALID;
}

/**
 * @brief Set a cursor to an error state.
 */
static void
cursor_set_error(flexi_cursor_s *cursor)
{
    cursor->buffer.data = "";
    cursor->buffer.length = 0;
    cursor->cursor = NULL;
    cursor->type = FLEXI_TYPE_INVALID;
    cursor->width = 0;
}

/**
 * @brief Return true if there is room in the buffer to read a length prefix.
 */
static bool
cursor_can_read_length_prefix(const flexi_cursor_s *cursor)
{
    return cursor->cursor - cursor->width >= cursor->buffer.data;
}

/**
 * @brief Given a cursor pointing at the base of a vector or vector-like
 *        type, obtain its length.
 *
 * @pre Type must be a type that has a length prefix.
 * @pre Buffer must have room for length at cursor position.
 *
 * @param[in] cursor Cursor to check.
 * @param[out] len Length of vector.
 * @return True if length was found.
 */
static bool
cursor_get_length_prefix_unsafe(const flexi_cursor_s *cursor,
    flexi_ssize_t *len)
{
    ASSERT(type_has_length_prefix(cursor->type));
    ASSERT(cursor_can_read_length_prefix(cursor));

    return buffer_read_size_by_width_unsafe(cursor->cursor - cursor->width,
        cursor->width, len);
}

/**
 * @brief Given a cursor pointing at the base of a vector or vector-like
 *        type, obtain its length.
 *
 * @pre Type must be a type that has a length prefix.
 *
 * @param[in] cursor Cursor to check.
 * @param[out] len Length of vector.
 * @return True if length was found.
 */
static bool
cursor_get_length_prefix(const flexi_cursor_s *cursor, flexi_ssize_t *len)
{
    ASSERT(type_has_length_prefix(cursor->type));

    if (!cursor_can_read_length_prefix(cursor)) {
        return false;
    }

    return buffer_read_size_by_width_unsafe(cursor->cursor - cursor->width,
        cursor->width, len);
}

/**
 * @brief Obtain the length of a fixed vector purely based on type.
 *
 * @param[in] cursor Cursor to check.
 * @param[out] len Length of vector.
 * @return True if length was found.
 */
static bool
cursor_get_fixed_len(const flexi_cursor_s *cursor, flexi_ssize_t *len)
{
    switch (cursor->type) {
    case FLEXI_TYPE_VECTOR_SINT2:
    case FLEXI_TYPE_VECTOR_UINT2:
    case FLEXI_TYPE_VECTOR_FLOAT2: *len = 2; return true;
    case FLEXI_TYPE_VECTOR_SINT3:
    case FLEXI_TYPE_VECTOR_UINT3:
    case FLEXI_TYPE_VECTOR_FLOAT3: *len = 3; return true;
    case FLEXI_TYPE_VECTOR_SINT4:
    case FLEXI_TYPE_VECTOR_UINT4:
    case FLEXI_TYPE_VECTOR_FLOAT4: *len = 4; return true;
    default: return false;
    }
}

/**
 * @brief Obtain the length of a cursor, no matter what type it is.
 *
 * @param[in] cursor Cursor to examine.
 * @return Length of cursor, or 0 if cursor has no length.
 */
static flexi_ssize_t
cursor_length(const flexi_cursor_s *cursor)
{
    flexi_ssize_t len = -1;
    if (type_has_length_prefix(cursor->type)) {
        // Get the length prefix.
        if (!cursor_get_length_prefix(cursor, &len)) {
            return 0;
        }
        return len;
    } else if (type_is_typed_vector_fixed(cursor->type)) {
        // Convert length type to return value.
        if (!cursor_get_fixed_len(cursor, &len)) {
            return 0;
        }
        return len;
    }

    return 0;
}

/**
 * @brief Given a cursor pointing at the base of a vector which contains
 *        trailing types, return a pointer to the first type.
 *
 * @warning This is a bug-prone function, be careful when working on it.
 *
 * @pre Cursor must be a map or untyped vector.
 * @pre Buffer must have room for length at cursor position.
 * @post Every vector value is in-bounds.
 *
 * @param[in] cursor Cursor to check.
 * @param[out] packed Pointer to first packed type.
 * @return True if types were located.
 */
static bool
cursor_vector_types_unsafe(const flexi_cursor_s *cursor,
    const flexi_packed_t **packed)
{
    ASSERT(type_is_map_or_untyped_vector(cursor->type));
    ASSERT(cursor_can_read_length_prefix(cursor));

    flexi_ssize_t len;
    if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
        return false;
    }

    // Make sure the destination pointer is in-bounds at all valid offsets.
    const char *dest = cursor->cursor + (len * cursor->width);
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
 * @pre Cursor type must be map or untyped vector.
 * @pre Buffer must have room for length at cursor position.
 *
 * @param[in] cursor Cursor to use as base.
 * @param[in] index Index to look up.
 * @param[out] dest Destination cursor.
 * @return True if seek was successful.
 */
static bool
cursor_seek_untyped_vector_index(const flexi_cursor_s *cursor,
    flexi_ssize_t index, flexi_cursor_s *dest)
{
    ASSERT(type_is_map_or_untyped_vector(cursor->type));
    ASSERT(cursor_can_read_length_prefix(cursor));

    const flexi_packed_t *types = NULL;
    if (!cursor_vector_types_unsafe(cursor, &types)) {
        return false;
    }

    flexi_type_e type = FLEXI_UNPACK_TYPE(types[index]);
    if (type_is_direct(type)) {
        // No need to resolve an offset, we're pretty much done.
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + (index * cursor->width);
        dest->type = type;
        dest->width = cursor->width;
        return true;
    }

    flexi_ssize_t offset = 0;
    const char *offset_ptr = cursor->cursor + (index * cursor->width);
    if (!buffer_read_size_by_width_unsafe(offset_ptr, cursor->width, &offset)) {
        return false;
    }

    if (!buffer_seek_back(&cursor->buffer, offset_ptr, offset, &dest->cursor)) {
        return false;
    }

    dest->buffer = cursor->buffer;
    dest->type = type;
    dest->width = FLEXI_WIDTH_TO_BYTES(FLEXI_UNPACK_WIDTH(types[index]));
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a typed vector, return a
 *        cursor pointing at the value at the given index.
 *
 * @param[in] cursor Cursor to use as base.
 * @param[in] index Index to look up.
 * @param[out] dest Destination cursor.
 * @return True if seek was successful.
 */
static bool
cursor_seek_typed_vector_index(const flexi_cursor_s *cursor,
    flexi_ssize_t index, flexi_cursor_s *dest)
{
    switch (cursor->type) {
    case FLEXI_TYPE_VECTOR_SINT:
    case FLEXI_TYPE_VECTOR_SINT2:
    case FLEXI_TYPE_VECTOR_SINT3:
    case FLEXI_TYPE_VECTOR_SINT4:
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + (index * cursor->width);
        dest->type = FLEXI_TYPE_SINT;
        dest->width = cursor->width;
        return true;
    case FLEXI_TYPE_VECTOR_UINT:
    case FLEXI_TYPE_VECTOR_UINT2:
    case FLEXI_TYPE_VECTOR_UINT3:
    case FLEXI_TYPE_VECTOR_UINT4:
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + (index * cursor->width);
        dest->type = FLEXI_TYPE_UINT;
        dest->width = cursor->width;
        return true;
    case FLEXI_TYPE_VECTOR_FLOAT:
    case FLEXI_TYPE_VECTOR_FLOAT2:
    case FLEXI_TYPE_VECTOR_FLOAT3:
    case FLEXI_TYPE_VECTOR_FLOAT4:
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + (index * cursor->width);
        dest->type = FLEXI_TYPE_FLOAT;
        dest->width = cursor->width;
        return true;
    case FLEXI_TYPE_VECTOR_BOOL:
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + index;
        dest->type = FLEXI_TYPE_BOOL;
        dest->width = 1;
        return true;
    case FLEXI_TYPE_VECTOR_KEY: {
        flexi_ssize_t offset;
        const char *cur = cursor->cursor + (index * cursor->width);
        if (!buffer_read_size_by_width_unsafe(cur, cursor->width, &offset)) {
            return false;
        }

        if (!buffer_seek_back(&cursor->buffer, cur, offset, &cur)) {
            return false;
        }

        dest->buffer = cursor->buffer;
        dest->cursor = cur;
        dest->type = FLEXI_TYPE_KEY;
        dest->width = 0;
        return true;
    }
    default: return false;
    }
}

/**
 * @brief Given a cursor pointing at the base of a map, return a cursor
 *        pointing at the keys vector of the map.
 *
 * @param[in] cursor Cursor to examine.
 * @param[out] dest Cursor which will be pointing at vector of keys.
 * @return True if seek was successful.
 */
static bool
cursor_map_keys(const flexi_cursor_s *cursor, flexi_cursor_s *dest)
{
    // Calling this with a non-map is a contract violation.
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    const char *cur;
    if (!buffer_seek_back(&cursor->buffer, cursor->cursor, cursor->width * 3,
            &cur)) {
        // Not enough room for the header.
        return false;
    }

    // [-3] contains key vector offset.
    flexi_ssize_t keys_offset;
    if (!buffer_read_size_by_width_unsafe(cur, cursor->width, &keys_offset)) {
        return false;
    }

    // [-2] contains key vector width.
    uint64_t keys_width =
        buffer_read_uint_unsafe(cur + cursor->width, cursor->width);

    if (!buffer_seek_back(&cursor->buffer, cur, keys_offset, &cur)) {
        // Tried to seek keys base, went out of bounds.
        return false;
    }

    dest->buffer = cursor->buffer;
    dest->cursor = cur;
    dest->type = FLEXI_TYPE_VECTOR_KEY;
    dest->width = (int)keys_width;
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a map, return the map key
 *        used by that index.
 *
 * @param[in] cursor Cursor to use as base.
 * @param[in] keys Cursor to previously sought-out keys.
 * @param[in] index Index to look up.
 * @param[out] str Key found by lookup.
 * @return True if lookup was successful.
 */
static bool
cursor_map_key_at_index(const flexi_cursor_s *cursor,
    const flexi_cursor_s *keys, flexi_ssize_t index, const char **str)
{
    // Calling this with a non-map is a contract violation.
    ASSERT(cursor->type == FLEXI_TYPE_MAP);
    ASSERT(keys->type == FLEXI_TYPE_VECTOR_KEY);

    // Resolve the key.
    flexi_ssize_t offset = 0;
    const char *offset_ptr = keys->cursor + (index * keys->width);
    if (!buffer_read_size_by_width(&cursor->buffer, offset_ptr, keys->width,
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
cursor_seek_map_key_linear(const flexi_cursor_s *cursor, flexi_ssize_t len,
    const char *key, flexi_cursor_s *dest)
{
    if (cursor->type != FLEXI_TYPE_MAP) {
        return FLEXI_ERR_INTERNAL;
    }

    flexi_cursor_s keys;
    if (!cursor_map_keys(cursor, &keys)) {
        // Could not locate keys.
        return FLEXI_ERR_BADREAD;
    }

    // Linear search.
    for (flexi_ssize_t i = 0; i < len; i++) {
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, &keys, i, &cmp)) {
            return FLEXI_ERR_BADREAD;
        }

        if (!strcmp(cmp, key)) {
            return cursor_seek_untyped_vector_index(cursor, i, dest)
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
cursor_seek_map_key_bsearch(const flexi_cursor_s *cursor, flexi_ssize_t len,
    const char *key, flexi_cursor_s *dest)
{
    if (cursor->type != FLEXI_TYPE_MAP) {
        return FLEXI_ERR_INTERNAL;
    }

    flexi_cursor_s keys;
    if (!cursor_map_keys(cursor, &keys)) {
        // Could not locate keys.
        return FLEXI_ERR_BADREAD;
    }

    flexi_ssize_t left = 0;
    flexi_ssize_t right = len - 1;
    while (left <= right) {
        flexi_ssize_t i = left + ((right - left) / 2);
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, &keys, i, &cmp)) {
            return FLEXI_ERR_BADREAD;
        }

        int res = strcmp(cmp, key);
        if (res == 0) {
            return cursor_seek_untyped_vector_index(cursor, i, dest)
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

/******************************************************************************/

static flexi_result_e
cursor_foreach_map(const flexi_cursor_s *cursor, flexi_foreach_fn foreach,
    void *user)
{
    flexi_cursor_s each;
    each.buffer = cursor->buffer;

    flexi_cursor_s keys;
    if (!cursor_map_keys(cursor, &keys)) {
        // Could not locate keys.
        return FLEXI_ERR_BADREAD;
    }

    flexi_ssize_t len;
    if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
        // There is no length prefix.
        return FLEXI_ERR_BADREAD;
    }

    const flexi_packed_t *types;
    if (!cursor_vector_types_unsafe(cursor, &types)) {
        // There is no types suffix.
        return FLEXI_ERR_BADREAD;
    }

    for (flexi_ssize_t i = 0; i < len; i++) {
        const char *key;
        if (!cursor_map_key_at_index(cursor, &keys, i, &key)) {
            // Couldn't find the map key.
            return FLEXI_ERR_BADREAD;
        }

        flexi_type_e type = FLEXI_UNPACK_TYPE(types[i]);
        if (type_is_direct(type)) {
            each.cursor = cursor->cursor + (i * cursor->width);
            each.type = type;
            each.width = cursor->width;
        } else {
            flexi_ssize_t offset = 0;
            const char *offset_ptr = cursor->cursor + (i * cursor->width);
            if (!buffer_read_size_by_width_unsafe(offset_ptr, cursor->width,
                    &offset)) {
                return FLEXI_ERR_BADREAD;
            }

            if (!buffer_seek_back(&cursor->buffer, offset_ptr, offset,
                    &each.cursor)) {
                return FLEXI_ERR_BADREAD;
            }

            each.type = type;
            each.width = FLEXI_WIDTH_TO_BYTES(FLEXI_UNPACK_WIDTH(types[i]));
        }

        if (!foreach (key, &each, user)) {
            return FLEXI_OK;
        }
    }

    return FLEXI_OK;
}

/******************************************************************************/

static flexi_result_e
cursor_foreach_untyped_vector(flexi_cursor_s *cursor, flexi_foreach_fn foreach,
    void *user)
{
    flexi_cursor_s each;
    each.buffer = cursor->buffer;

    flexi_ssize_t len;
    if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    const flexi_packed_t *types;
    if (!cursor_vector_types_unsafe(cursor, &types)) {
        return FLEXI_ERR_BADREAD;
    }

    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_type_e type = FLEXI_UNPACK_TYPE(types[i]);
        if (type_is_direct(type)) {
            each.cursor = cursor->cursor + (i * cursor->width);
            each.type = type;
            each.width = cursor->width;
        } else {
            flexi_ssize_t offset = 0;
            const char *offset_ptr = cursor->cursor + (i * cursor->width);
            if (!buffer_read_size_by_width_unsafe(offset_ptr, cursor->width,
                    &offset)) {
                return FLEXI_ERR_BADREAD;
            }

            if (!buffer_seek_back(&cursor->buffer, offset_ptr, offset,
                    &each.cursor)) {
                return FLEXI_ERR_BADREAD;
            }

            each.type = type;
            each.width = FLEXI_WIDTH_TO_BYTES(FLEXI_UNPACK_WIDTH(types[i]));
        }

        if (!foreach (NULL, &each, user)) {
            return FLEXI_OK;
        }
    }

    return FLEXI_OK;
}

/**
 * @brief Call the float callback.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK || FLEXI_ERR_BADREAD.
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

/******************************************************************************/

typedef struct foreach_ctx_s {
    const flexi_parser_s *parser;
    void *user;
    parse_limits_s *limits;
    flexi_result_e err;
} foreach_ctx_s;

static bool
parser_emit_foreach(const char *key, flexi_cursor_s *value, void *user)
{
    foreach_ctx_s *ctx = (foreach_ctx_s *)user;

    ctx->limits->depth += 1;
    flexi_result_e res =
        parse_cursor(ctx->parser, key, value, ctx->user, ctx->limits);
    ctx->limits->depth -= 1;

    if (FLEXI_ERROR(res)) {
        ctx->err = res;
        return false;
    }

    return true;
}

/**
 * @brief Call the map callbacks and iterate map values.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @param[in] limits Parse limit tracking.
 * @return FLEXI_OK || FLEXI_ERR_BADREAD.
 */
static flexi_result_e
parser_emit_map(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user, parse_limits_s *limits)
{
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    flexi_ssize_t len;
    if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    foreach_ctx_s ctx;
    ctx.parser = parser;
    ctx.user = user;
    ctx.limits = limits;
    ctx.err = FLEXI_INVALID;

    parser->map_begin(key, len, user);
    if (!cursor_foreach_map(cursor, parser_emit_foreach, &ctx)) {
        return ctx.err;
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
 * @return FLEXI_OK || FLEXI_ERR_BADREAD || FLEXI_ERR_PARSELIMIT.
 */
static flexi_result_e
parser_emit_vector(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user, parse_limits_s *limits)
{
    ASSERT(cursor->type == FLEXI_TYPE_VECTOR);

    flexi_ssize_t len;
    if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    flexi_result_e res;
    parser->vector_begin(key, len, user);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s value;
        if (!cursor_seek_untyped_vector_index(cursor, i, &value)) {
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
 * @brief Call the vector callbacks and iterate keys in vector.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of value.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @return FLEXI_OK || FLEXI_ERR_BADREAD.
 */
static flexi_result_e
parser_emit_vector_keys(const flexi_parser_s *parser, const char *key,
    const flexi_cursor_s *cursor, void *user)
{
    ASSERT(cursor->type == FLEXI_TYPE_VECTOR_KEY);

    flexi_ssize_t len;
    if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
        return FLEXI_ERR_BADREAD;
    }

    parser->vector_begin(key, len, user);
    for (flexi_ssize_t i = 0; i < len; i++) {
        // Access the offset by hand.
        flexi_ssize_t offset = 0;
        const char *offset_ptr = cursor->cursor + (i * cursor->width);
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
 * @brief Call the appropriate parser callbacks at the given cursor.
 *
 * @param[in] parser Parser to operate on.
 * @param[in] key Key of cursor.
 * @param[in] cursor Location of cursor to read with.
 * @param[in] user User pointer.
 * @param[in] limits Parse limit tracking.
 * @return FLEXI_OK || FLEXI_ERR_BADREAD || FLEXI_ERR_PARSELIMIT.
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
    case FLEXI_TYPE_INDIRECT_SINT: {
        int64_t v;
        if (!buffer_read_sint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->sint(key, v, user);
        return FLEXI_OK;
    }
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_UINT: {
        uint64_t v;
        if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->uint(key, v, user);
        return FLEXI_OK;
    }
    case FLEXI_TYPE_FLOAT:
    case FLEXI_TYPE_INDIRECT_FLOAT:
        return parser_emit_flt(parser, key, cursor, user);
    case FLEXI_TYPE_KEY:
        parser->key(key, cursor->cursor, user);
        return FLEXI_OK;
    case FLEXI_TYPE_STRING: {
        flexi_ssize_t len;
        if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->string(key, cursor->cursor, len, user);
        return FLEXI_OK;
    }
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
    case FLEXI_TYPE_VECTOR_BOOL: {
        flexi_ssize_t count;
        if (!cursor_get_length_prefix_unsafe(cursor, &count)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->typed_vector(key, cursor->cursor, cursor->type, cursor->width,
            count, user);
        return FLEXI_OK;
    }
    case FLEXI_TYPE_VECTOR_KEY:
        return parser_emit_vector_keys(parser, key, cursor, user);
    case FLEXI_TYPE_VECTOR_SINT2:
    case FLEXI_TYPE_VECTOR_UINT2:
    case FLEXI_TYPE_VECTOR_FLOAT2:
        parser->typed_vector(key, cursor->cursor, cursor->type, cursor->width,
            2, user);
        return FLEXI_OK;
    case FLEXI_TYPE_VECTOR_SINT3:
    case FLEXI_TYPE_VECTOR_UINT3:
    case FLEXI_TYPE_VECTOR_FLOAT3:
        parser->typed_vector(key, cursor->cursor, cursor->type, cursor->width,
            3, user);
        return FLEXI_OK;
    case FLEXI_TYPE_VECTOR_SINT4:
    case FLEXI_TYPE_VECTOR_UINT4:
    case FLEXI_TYPE_VECTOR_FLOAT4:
        parser->typed_vector(key, cursor->cursor, cursor->type, cursor->width,
            4, user);
        return FLEXI_OK;
    case FLEXI_TYPE_BLOB: {
        flexi_ssize_t len;
        if (!cursor_get_length_prefix_unsafe(cursor, &len)) {
            return FLEXI_ERR_BADREAD;
        }

        parser->blob(key, cursor->cursor, len, user);
        return FLEXI_OK;
    }
    case FLEXI_TYPE_BOOL:
        parser->boolean(key, *(const bool *)(cursor->cursor), user);
        return FLEXI_OK;
    }

    return FLEXI_ERR_INTERNAL;
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
writer_peek_tail(flexi_writer_s *writer, flexi_ssize_t offset)
{
    return stack_at(&writer->stack, offset);
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
writer_peek_idx(flexi_writer_s *writer, flexi_ssize_t start,
    flexi_ssize_t index)
{
    flexi_ssize_t offset = stack_count(&writer->stack);

    offset -= start;
    offset += index;

    return stack_at(&writer->stack, offset);
}

/**
 * @brief Pop a value off the stack.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] count Number of values to pop.
 * @return True if pop was successful, or false if stack was underflowed.
 */
static bool
writer_pop(flexi_writer_s *writer, flexi_ssize_t count)
{
    flexi_ssize_t scount = stack_count(&writer->stack);
    if (scount < count) {
        return false;
    }

    flexi_ssize_t actual = stack_pop(&writer->stack, count);
    return count == actual;
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
writer_get_stack_key(flexi_writer_s *writer, flexi_ssize_t start,
    flexi_ssize_t index)
{
    flexi_ssize_t v_index = start + index;
    flexi_value_s *v = stack_at(&writer->stack, v_index);
    if (v->type != FLEXI_TYPE_KEY) {
        return NULL;
    }
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
    flexi_ssize_t index)
{
    flexi_ssize_t v_index = start + index;
    return stack_at(&writer->stack, v_index);
}

/**
 * @brief Sort the values of a map so all values keys are in strcmp order.
 *
 * @param writer[in] Writer to operate on.
 * @param len[in] Number of values to use for map.
 * @return True if sort was successful.
 */
static bool
writer_sort_map_values(flexi_writer_s *writer, flexi_ssize_t len)
{
    flexi_ssize_t start = stack_count(&writer->stack) - len;
    if (start < 0) {
        return false;
    }

    // TODO: Use a faster sort for large key sets.
    for (flexi_ssize_t i = 1; i < len; i++) {
        flexi_value_s cur = *writer_get_stack_value(writer, start, i);
        const char *curkey = cur.key;

        flexi_ssize_t j = i;
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

    return true;
}

/**
 * @brief Calculate the minimum alignment needed to store values in vector.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] len Number of values to examine.
 * @param[out] min Actual minimum alignment needed in bytes.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE.
 */
static flexi_result_e
writer_vector_calc_min_stride(flexi_writer_s *writer, flexi_ssize_t len,
    int *min)
{
    int min_width = 1;
    for (flexi_ssize_t i = 0; i < len; i++) {
        const flexi_value_s *value = writer_peek_idx(writer, len, i);
        if (value == NULL) {
            return FLEXI_ERR_INTERNAL;
        }

        if (value->type == FLEXI_TYPE_SINT || value->type == FLEXI_TYPE_UINT ||
            value->type == FLEXI_TYPE_FLOAT) {
            // Trivial.
            min_width = MAX(min_width, value->width);
        } else if (type_is_indirect(value->type)) {
            // Get the current cursor position - we haven't written any data
            // yet, so this will be pre-padding and pre-length.
            flexi_ssize_t current;
            if (!ostream_tell(&writer->ostream, &current)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Calculate an offset relative to the start of the array.
            flexi_ssize_t start_offset = current - value->u.offset;

            // The stride of the vector has to be large enough to contain
            // the length of the vector and the offset *calculated from the
            // position inside the array*.  Hold on to your hats...

            int start_offset_width = UINT_WIDTH(start_offset);
            int len_width = UINT_WIDTH(len);
            int check_stride = MAX(min_width, start_offset_width);
            check_stride = MAX(check_stride, len_width);

            for (;;) {
                // Estimate the final offset with the current checked width.
                // We're accounting for vector's length prefix and one extra
                // stride's worth of padding.
                flexi_ssize_t offset = start_offset + (check_stride * (i + 2));
                int offset_width = UINT_WIDTH(offset);
                if (offset_width <= check_stride) {
                    break;
                }

                check_stride <<= 1;
                if (check_stride > 8) {
                    return FLEXI_ERR_INTERNAL;
                }
            }

            min_width = MAX(min_width, check_stride);
        }
    }

    *min = min_width;
    return FLEXI_OK;
}

/**
 * @brief Write out the values of a vector.
 *
 * @param[in] writer Writer to operate on.
 * @param[in] len Number of values to write.
 * @param[in] stride Number of bytes between values.
 * @return FLEXI_OK || FLEXI_ERR_BADWRITE.
 */
static flexi_result_e
write_vector_values(flexi_writer_s *writer, flexi_ssize_t len, int stride)
{
    // Write values
    for (flexi_ssize_t i = 0; i < len; i++) {
        const flexi_value_s *value = writer_peek_idx(writer, len, i);
        if (value == NULL) {
            return FLEXI_ERR_INTERNAL;
        }

        if (value->type == FLEXI_TYPE_SINT) {
            // Write value inline.
            if (!write_sint_by_width(writer, value->u.s64, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
        } else if (value->type == FLEXI_TYPE_UINT) {
            // Write value inline.
            if (!write_uint_by_width(writer, value->u.u64, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
        } else if (value->type == FLEXI_TYPE_FLOAT) {
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
        } else if (type_is_indirect(value->type)) {
            // Get the current cursor position.
            flexi_ssize_t current;
            if (!ostream_tell(&writer->ostream, &current)) {
                return FLEXI_ERR_BADWRITE;
            }

            // Calculate and write offset.
            flexi_ssize_t offset = current - value->u.offset;
            if (!write_uint_by_width(writer, offset, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
        } else if (value->type == FLEXI_TYPE_BOOL) {
            // Write value inline.
            if (!write_uint_by_width(writer, value->u.u64, stride)) {
                return FLEXI_ERR_BADWRITE;
            }
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
write_vector_types(flexi_writer_s *writer, flexi_ssize_t len)
{
    for (int i = 0; i < (int)len; i++) {
        const flexi_value_s *value = writer_peek_idx(writer, (int)len, i);
        flexi_packed_t packed =
            PACK_TYPE(value->type) | PACK_WIDTH(value->width);
        if (!ostream_write(&writer->ostream, &packed, sizeof(flexi_packed_t))) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Sort the keys of a map so all keys are in strcmp order.
 *
 * @param writer[in] Writer to operate on.
 * @param len[in] Number of keys to use for map.
 * @return True if sort was successful.
 */
static bool
writer_sort_map_keys(flexi_writer_s *writer, flexi_ssize_t len)
{
    flexi_ssize_t start = stack_count(&writer->stack) - len;
    if (start < 0) {
        return false;
    }

    // TODO: Use a faster sort for large key sets.
    for (flexi_ssize_t i = 1; i < len; i++) {
        flexi_value_s *cur = writer_get_stack_key(writer, start, i);
        flexi_ssize_t curoff = cur->u.offset;
        const char *curkey =
            (const char *)ostream_data_at(&writer->ostream, curoff);

        flexi_ssize_t j = i;
        for (; j > 0; j--) {
            flexi_value_s *seek = writer_get_stack_key(writer, start, j - 1);
            const char *seekkey =
                (const char *)ostream_data_at(&writer->ostream, seek->u.offset);

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

    return true;
}

/******************************************************************************/

static flexi_result_e
write_key(flexi_writer_s *writer, const char *key, const char *str)
{
    // Keep track of string starting position.
    flexi_ssize_t offset;
    if (!ostream_tell(&writer->ostream, &offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write the string, plus the trailing '\0'.
    flexi_ssize_t len = strlen(str);
    if (!ostream_write(&writer->ostream, str, len + 1)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Push offset to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_KEY;
    stack->width = 0;
    return FLEXI_OK;
}

/******************************************************************************/

static flexi_result_e
write_map_keys(flexi_writer_s *writer, flexi_ssize_t len, flexi_width_e stride,
    flexi_stack_idx_t *keyset)
{
    // Find the start of the keys on the stack.
    flexi_ssize_t start = stack_count(&writer->stack) - len;
    if (start < 0) {
        return FLEXI_ERR_BADSTACK;
    }

    // All keys must be of key type.  Avoids redundant checks later.
    for (flexi_ssize_t i = start; i < stack_count(&writer->stack); i++) {
        flexi_value_s *value = stack_at(&writer->stack, i);
        if (value->type != FLEXI_TYPE_KEY) {
            return FLEXI_ERR_NOTKEYS;
        }
    }

    // Sort the keys by key name.
    writer_sort_map_keys(writer, len);

    int stride_bytes = FLEXI_WIDTH_TO_BYTES(stride);

    // Write length
    if (!write_uint_by_width(writer, len, stride_bytes)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Keep track of the base location of the keys.
    flexi_ssize_t keys_offset;
    if (!ostream_tell(&writer->ostream, &keys_offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write out key offsets.
    for (flexi_ssize_t i = start; i < stack_count(&writer->stack); i++) {
        flexi_value_s *value = stack_at(&writer->stack, i);
        if (value->type != FLEXI_TYPE_KEY) {
            return FLEXI_ERR_INTERNAL;
        }

        // TODO: We can pre-calculate this.
        flexi_ssize_t current;
        if (!ostream_tell(&writer->ostream, &current)) {
            return FLEXI_ERR_BADWRITE;
        }

        flexi_ssize_t offset = current - value->u.offset;
        if (!write_uint_by_width(writer, offset, stride_bytes)) {
            return FLEXI_ERR_BADWRITE;
        }
    }

    bool ok = writer_pop(writer, len);
    if (!ok) {
        return FLEXI_ERR_BADSTACK;
    }

    if (keyset != NULL) {
        *keyset = stack_count(&writer->stack);
    }

    flexi_value_s *stack = stack_push(&writer->stack);
    if (stack == NULL) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->type = FLEXI_TYPE_VECTOR_KEY;
    stack->u.offset = keys_offset;
    stack->width = stride_bytes;
    stack->key = NULL;
    return FLEXI_OK;
}

/******************************************************************************/

static flexi_result_e
write_map_values(flexi_writer_s *writer, const char *key,
    flexi_stack_idx_t keyset, flexi_ssize_t len, flexi_width_e stride)
{
    // First seek out the keys.
    flexi_value_s *keys_value = writer_peek_tail(writer, keyset);
    if (keys_value == NULL || keys_value->type != FLEXI_TYPE_VECTOR_KEY) {
        return FLEXI_ERR_BADTYPE;
    }

    // Sort the values based on their keys.
    if (!writer_sort_map_values(writer, len)) {
        return FLEXI_ERR_INTERNAL;
    }

    // The stride the developer passed in might not be wide enough to contain
    // all of the values.  Calculate a minimum stride.
    int min_stride_bytes;
    flexi_result_e res =
        writer_vector_calc_min_stride(writer, len, &min_stride_bytes);
    if (FLEXI_ERROR(res)) {
        return res;
    }

    // Take the larger of our calculation vs. parameter.
    int stride_bytes = FLEXI_WIDTH_TO_BYTES(stride);
    stride_bytes = MAX(stride_bytes, min_stride_bytes);

    // Offset to aligned keys vector.
    flexi_ssize_t current;
    if (!write_padding(writer, 0, stride_bytes, &current)) {
        return FLEXI_ERR_BADWRITE;
    }

    flexi_ssize_t offset = current - keys_value->u.offset;
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
    flexi_ssize_t values_offset;
    if (!ostream_tell(&writer->ostream, &values_offset)) {
        return FLEXI_ERR_BADWRITE;
    }

    // Write values.
    res = write_vector_values(writer, len, stride_bytes);
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
    flexi_value_s *stack = stack_push(&writer->stack);
    if (stack == NULL) {
        return FLEXI_ERR_BADSTACK;
    }

    stack->u.offset = values_offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_MAP;
    stack->width = stride_bytes;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_buffer_s
flexi_make_buffer(const void *buffer, flexi_ssize_t len)
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
        cursor_set_error(cursor);
        return FLEXI_ERR_BADREAD;
    }

    // Width of root object.
    cursor->buffer = *buffer;
    cursor->cursor = buffer->data + buffer->length - 1;
    uint8_t root_bytes = *(const uint8_t *)(cursor->cursor);
    if (root_bytes == 0 || (size_t)buffer->length < root_bytes + 2u) {
        cursor_set_error(cursor);
        return FLEXI_ERR_BADREAD;
    }

    // Obtain the packed type.
    cursor->cursor -= 1;
    flexi_packed_t packed = *(const uint8_t *)(cursor->cursor);

    // Point at the root object.
    flexi_type_e type = FLEXI_UNPACK_TYPE(packed);
    cursor->cursor -= root_bytes;
    if (type_is_direct(type)) {
        // No need to resolve an offset, we're done.
        cursor->type = type;
        cursor->width = root_bytes;
        return FLEXI_OK;
    }

    // We're pointing at an offset, resolve it.
    flexi_ssize_t offset = 0;
    if (!buffer_read_size_by_width(&cursor->buffer, cursor->cursor, root_bytes,
            &offset)) {
        cursor_set_error(cursor);
        return FLEXI_ERR_BADREAD;
    }

    const char *dest = NULL;
    if (!buffer_seek_back(&cursor->buffer, cursor->cursor, offset, &dest)) {
        cursor_set_error(cursor);
        return FLEXI_ERR_BADREAD;
    }

    cursor->cursor = dest;
    cursor->type = type;
    cursor->width = FLEXI_WIDTH_TO_BYTES(FLEXI_UNPACK_WIDTH(packed));
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

flexi_ssize_t
flexi_cursor_length(const flexi_cursor_s *cursor)
{
    return cursor_length(cursor);
}

/******************************************************************************/

flexi_result_e
flexi_cursor_sint(const flexi_cursor_s *cursor, int64_t *val)
{
    if (cursor_is_error(cursor)) {
        *val = 0;
        return FLEXI_ERR_FAILSAFE;
    }

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
        if (v > (float)INT64_MAX) {
            *val = INT64_MAX;
            return FLEXI_ERR_RANGE;
        }
        if (v < (float)INT64_MIN) {
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
        if (v > (double)INT64_MAX) {
            *val = INT64_MAX;
            return FLEXI_ERR_RANGE;
        }
        if (v < (double)INT64_MIN) {
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
    if (cursor_is_error(cursor)) {
        *val = 0;
        return FLEXI_ERR_FAILSAFE;
    }

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
    if (cursor_is_error(cursor)) {
        *val = 0.0f;
        return FLEXI_ERR_FAILSAFE;
    }

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
    if (cursor_is_error(cursor)) {
        *val = 0.0;
        return FLEXI_ERR_FAILSAFE;
    }

    if (type_is_sint(cursor->type)) {
        int64_t v;
        if (!buffer_read_sint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0;
            return FLEXI_ERR_BADREAD;
        }

        *val = (double)v;
        return FLEXI_OK;
    } else if (type_is_uint(cursor->type)) {
        uint64_t v;
        if (!buffer_read_uint(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0;
            return FLEXI_ERR_BADREAD;
        }

        *val = (double)v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 4) {
        float v;
        if (!buffer_read_f32(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0;
            return FLEXI_ERR_BADREAD;
        }

        *val = v;
        return FLEXI_OK;
    } else if (type_is_flt(cursor->type) && cursor->width == 8) {
        double v;
        if (!buffer_read_f64(&cursor->buffer, cursor->cursor, cursor->width,
                &v)) {
            *val = 0.0;
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
    if (cursor_is_error(cursor)) {
        *str = "";
        return FLEXI_ERR_FAILSAFE;
    }

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
flexi_cursor_string(const flexi_cursor_s *cursor, const char **str,
    flexi_ssize_t *len)
{
    if (cursor_is_error(cursor)) {
        *str = "";
        *len = 0;
        return FLEXI_ERR_FAILSAFE;
    }

    switch (cursor->type) {
    case FLEXI_TYPE_KEY: {
        *str = cursor->cursor;
        *len = strlen(cursor->cursor);
        return FLEXI_OK;
    }
    case FLEXI_TYPE_STRING: {
        flexi_ssize_t l;
        if (!cursor_get_length_prefix_unsafe(cursor, &l)) {
            *str = "";
            *len = 0;
            return FLEXI_ERR_BADREAD;
        } else if (l < 0) {
            *str = "";
            *len = 0;
            return FLEXI_ERR_INTERNAL;
        }

        *str = cursor->cursor;
        *len = l;
        return FLEXI_OK;
    }
    default: {
        *str = "";
        *len = 0;
        return FLEXI_ERR_BADTYPE;
    }
    }
}

/******************************************************************************/

flexi_result_e
flexi_cursor_map_key_at_index(const flexi_cursor_s *cursor, flexi_ssize_t index,
    const char **str)
{
    if (cursor_is_error(cursor)) {
        *str = "";
        return FLEXI_ERR_FAILSAFE;
    }

    if (cursor->type != FLEXI_TYPE_MAP) {
        *str = "";
        return FLEXI_ERR_BADTYPE;
    }

    flexi_cursor_s keys;
    if (!cursor_map_keys(cursor, &keys)) {
        *str = "";
        return FLEXI_ERR_BADREAD;
    }

    if (!cursor_map_key_at_index(cursor, &keys, index, str)) {
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
    if (cursor_is_error(cursor)) {
        cursor_set_error(dest);
        return FLEXI_ERR_FAILSAFE;
    }

    if (cursor->type != FLEXI_TYPE_MAP) {
        cursor_set_error(dest);
        return FLEXI_ERR_BADTYPE;
    }

    flexi_ssize_t len;
    if (!cursor_get_length_prefix(cursor, &len)) {
        cursor_set_error(dest);
        return FLEXI_ERR_BADREAD;
    }

    if (len <= FLEXI_CONFIG_SEEK_MAP_KEY_LINEAR_MAX) {
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
    if (cursor_is_error(cursor)) {
        *packed = &g_empty_packed;
        return FLEXI_ERR_FAILSAFE;
    }

    if (cursor->type != FLEXI_TYPE_VECTOR && cursor->type != FLEXI_TYPE_MAP) {
        *packed = &g_empty_packed;
        return FLEXI_ERR_BADTYPE;
    }

    if (!cursor_can_read_length_prefix(cursor)) {
        *packed = &g_empty_packed;
        return FLEXI_ERR_BADREAD;
    }

    if (!cursor_vector_types_unsafe(cursor, packed)) {
        *packed = &g_empty_packed;
        return FLEXI_ERR_INTERNAL;
    }

    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor,
    flexi_ssize_t index, flexi_cursor_s *dest)
{
    if (cursor_is_error(cursor)) {
        cursor_set_error(dest);
        return FLEXI_ERR_FAILSAFE;
    }

    if (type_is_map_or_untyped_vector(cursor->type)) {
        // Untyped vectors and maps can be seeked.
        if (!cursor_seek_untyped_vector_index(cursor, index, dest)) {
            cursor_set_error(dest);
            return FLEXI_ERR_INTERNAL;
        }

        return FLEXI_OK;
    } else if (type_is_typed_vector(cursor->type)) {
        // Typed vectors can still be seeked.  It's slower than direct access,
        // but it should still work.
        if (!cursor_seek_typed_vector_index(cursor, index, dest)) {
            cursor_set_error(dest);
            return FLEXI_ERR_INTERNAL;
        }

        return FLEXI_OK;
    }

    cursor_set_error(dest);
    return FLEXI_ERR_BADTYPE;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_typed_vector_data(const flexi_cursor_s *cursor, const void **data,
    flexi_type_e *type, int *stride, flexi_ssize_t *count)
{
    if (cursor_is_error(cursor)) {
        *data = &g_empty_typed_vector;
        *type = FLEXI_TYPE_INVALID;
        *stride = 0;
        *count = 0;
        return FLEXI_ERR_FAILSAFE;
    }

    if (type_is_typed_vector(cursor->type)) {
        *data = cursor->cursor;
        *type = cursor->type;
        *stride = cursor->width;
        *count = cursor_length(cursor);
        return FLEXI_OK;
    }

    *data = &g_empty_typed_vector;
    *type = FLEXI_TYPE_INVALID;
    *stride = 0;
    *count = 0;
    return FLEXI_ERR_BADTYPE;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_foreach(flexi_cursor_s *cursor, flexi_foreach_fn foreach,
    void *user)
{
    switch (cursor->type) {
    case FLEXI_TYPE_MAP: return cursor_foreach_map(cursor, foreach, user);
    case FLEXI_TYPE_VECTOR:
        return cursor_foreach_untyped_vector(cursor, foreach, user);
    default: return FLEXI_ERR_BADTYPE;
    }
}

/******************************************************************************/

flexi_result_e
flexi_cursor_blob(const flexi_cursor_s *cursor, const uint8_t **blob,
    flexi_ssize_t *len)
{
    if (cursor_is_error(cursor)) {
        *blob = &g_empty_blob;
        *len = 0;
        return FLEXI_ERR_FAILSAFE;
    }

    if (cursor->type != FLEXI_TYPE_BLOB) {
        *blob = &g_empty_blob;
        *len = 0;
        return FLEXI_ERR_BADTYPE;
    }

    if (!cursor_get_length_prefix_unsafe(cursor, len)) {
        *blob = &g_empty_blob;
        *len = 0;
        return FLEXI_ERR_BADREAD;
    }

    *blob = (const uint8_t *)cursor->cursor;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_cursor_bool(const flexi_cursor_s *cursor, bool *val)
{
    if (cursor_is_error(cursor)) {
        *val = false;
        return FLEXI_ERR_FAILSAFE;
    }

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

flexi_stack_s
flexi_make_stack(flexi_stack_at_fn at, flexi_stack_count_fn count,
    flexi_stack_push_fn push, flexi_stack_pop_fn pop, void *user)
{
    flexi_stack_s stack;
    stack.at = at;
    stack.count = count;
    stack.push = push;
    stack.pop = pop;
    stack.user = user;
    return stack;
}

/******************************************************************************/

flexi_ostream_s
flexi_make_ostream(flexi_ostream_write_fn write,
    flexi_ostream_data_at_fn data_at, flexi_ostream_tell_fn tell, void *user)
{
    flexi_ostream_s ostream;
    ostream.write = write;
    ostream.data_at = data_at;
    ostream.tell = tell;
    ostream.user = user;
    return ostream;
}

/******************************************************************************/

flexi_writer_s
flexi_make_writer(const flexi_stack_s *stack, const flexi_ostream_s *ostream)
{
    flexi_writer_s writer;
    writer.stack = *stack;
    writer.ostream = *ostream;
    writer.err = FLEXI_INVALID;
    return writer;
}

/******************************************************************************/

flexi_result_e
flexi_write_null(flexi_writer_s *writer, const char *key)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Push value to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.u64 = 0;
    stack->key = key;
    stack->type = FLEXI_TYPE_NULL;
    stack->width = 1;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_sint(flexi_writer_s *writer, const char *key, int64_t v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Push value to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.u64 = v;
    stack->key = key;
    stack->type = FLEXI_TYPE_SINT;
    stack->width = SINT_WIDTH(v);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_uint(flexi_writer_s *writer, const char *key, uint64_t value)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Push value to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.u64 = value;
    stack->key = key;
    stack->type = FLEXI_TYPE_UINT;
    stack->width = UINT_WIDTH(value);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_f32(flexi_writer_s *writer, const char *key, float v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Push value to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.f32 = v;
    stack->key = key;
    stack->type = FLEXI_TYPE_FLOAT;
    stack->width = sizeof(float);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_f64(flexi_writer_s *writer, const char *key, double v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Push value to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.f64 = v;
    stack->key = key;
    stack->type = FLEXI_TYPE_FLOAT;
    stack->width = sizeof(double);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_key(flexi_writer_s *writer, const char *str)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    flexi_result_e res = write_key(writer, NULL, str);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
    }

    return res;
}

/******************************************************************************/

flexi_result_e
flexi_write_keyed_key(flexi_writer_s *writer, const char *key, const char *str)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    flexi_result_e res = write_key(writer, key, str);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
    }

    return res;
}

/******************************************************************************/

flexi_result_e
flexi_write_string(flexi_writer_s *writer, const char *key, const char *str,
    size_t ulen)
{
    flexi_ssize_t len = (flexi_ssize_t)ulen;

    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Write the string length to stream.
    int width = UINT_WIDTH(len);
    if (!write_uint_by_width(writer, len, width)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Keep track of string starting position.
    flexi_ssize_t offset;
    if (!ostream_tell(&writer->ostream, &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write the string, plus the trailing '\0'.
    if (!ostream_write(&writer->ostream, str, len + 1)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Push offset to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_STRING;
    stack->width = width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_strlen(flexi_writer_s *writer, const char *key, const char *str)
{
    return flexi_write_string(writer, key, str, strlen(str));
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_sint(flexi_writer_s *writer, const char *key, int64_t v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    int width = SINT_WIDTH(v);

    // Align to the nearest multiple.
    flexi_ssize_t offset;
    if (!write_padding(writer, 0, width, &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_sint_by_width(writer, v, width)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Push offset to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_INDIRECT_SINT;
    stack->width = width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_uint(flexi_writer_s *writer, const char *key, uint64_t v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    int width = UINT_WIDTH(v);

    // Align to the nearest multiple.
    flexi_ssize_t offset;
    if (!write_padding(writer, 0, width, &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_uint_by_width(writer, v, width)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Push offset to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_INDIRECT_UINT;
    stack->width = width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_f32(flexi_writer_s *writer, const char *key, float v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Align to the nearest multiple.
    flexi_ssize_t offset;
    if (!write_padding(writer, 0, sizeof(float), &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_f32(writer, v, sizeof(float))) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Push offset to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_INDIRECT_FLOAT;
    stack->width = sizeof(float);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_indirect_f64(flexi_writer_s *writer, const char *key, double v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Keep track of the value's position.
    flexi_ssize_t offset;
    if (!write_padding(writer, 0, sizeof(double), &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_f64(writer, v, sizeof(double))) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Push offset to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_INDIRECT_FLOAT;
    stack->width = sizeof(double);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_map_keys(flexi_writer_s *writer, flexi_ssize_t len,
    flexi_width_e stride, flexi_stack_idx_t *keyset)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    flexi_result_e res = write_map_keys(writer, len, stride, keyset);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
    }

    return res;
}

/******************************************************************************/

flexi_result_e
flexi_write_map_values(flexi_writer_s *writer, const char *key,
    flexi_stack_idx_t keyset, flexi_ssize_t len, flexi_width_e stride)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    flexi_result_e res = write_map_values(writer, key, keyset, len, stride);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
    }

    return res;
}

/******************************************************************************/

flexi_result_e
flexi_write_map(flexi_writer_s *writer, const char *key, flexi_ssize_t len,
    flexi_width_e stride)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    flexi_ssize_t values_end = stack_count(&writer->stack);
    flexi_ssize_t values_start = values_end - len;
    if (values_start < 0) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    // Push keys to the stack.
    flexi_result_e res;
    for (flexi_ssize_t i = values_start; i < values_end; i++) {
        flexi_value_s *value = stack_at(&writer->stack, i);
        if (value == NULL) {
            writer->err = FLEXI_ERR_INTERNAL;
            return writer->err;
        }

        res = write_key(writer, NULL, value->key);
        if (FLEXI_ERROR(res)) {
            writer->err = res;
            return writer->err;
        }
    }

    // Push the key array to the stack.
    res = write_map_keys(writer, len, FLEXI_WIDTH_1B, NULL);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
        return writer->err;
    }

    // We need to move the keys lower on the stack than values.
    bool ok = stack_roll_down(&writer->stack, values_start);
    if (!ok) {
        writer->err = FLEXI_ERR_INTERNAL;
        return writer->err;
    }

    // Write out all of our values.
    res = write_map_values(writer, key, values_start, len, stride);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
        return writer->err;
    }

    // Pop the key array from the stack - we don't keep it around.
    ok = stack_swap(&writer->stack);
    if (!ok) {
        writer->err = FLEXI_ERR_INTERNAL;
        return writer->err;
    }

    ok = stack_pop(&writer->stack, 1) == 1;
    if (!ok) {
        writer->err = FLEXI_ERR_INTERNAL;
        return writer->err;
    }

    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_vector(flexi_writer_s *writer, const char *key, flexi_ssize_t len,
    flexi_width_e stride)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // The stride the developer passed in might not be wide enough to contain
    // all of the values.  Calculate a minimum stride.
    int min_stride_bytes;
    flexi_result_e res =
        writer_vector_calc_min_stride(writer, len, &min_stride_bytes);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
        return writer->err;
    }

    // Take the larger of our calculation vs. parameter.
    int stride_bytes = FLEXI_WIDTH_TO_BYTES(stride);
    stride_bytes = MAX(stride_bytes, min_stride_bytes);

    // Align future writes to the nearest multiple.
    flexi_ssize_t offset;
    if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write length
    if (!write_uint_by_width(writer, len, stride_bytes)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write values.
    res = write_vector_values(writer, len, stride_bytes);
    if (FLEXI_ERROR(res)) {
        writer->err = res;
        return writer->err;
    }

    // Write types.
    if (!write_vector_types(writer, len)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Pop values.
    bool ok = writer_pop(writer, len);
    if (!ok) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    // Push vector.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (stack == NULL) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_VECTOR;
    stack->width = stride_bytes;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_sint(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, flexi_ssize_t len)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    int stride_bytes = FLEXI_WIDTH_TO_BYTES(stride);
    if (!(stride_bytes >= 1 && stride_bytes <= 8)) {
        return FLEXI_ERR_PARAM;
    }

    if (len >= 2 && len <= 4) {
        // Keep track of the aligned base location of the vector.
        flexi_ssize_t offset;
        if (!write_padding(writer, 0, stride_bytes, &offset)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write values.
        if (!ostream_write(&writer->ostream, ptr, len * stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        flexi_value_s *stack = stack_push(&writer->stack);
        if (stack == NULL) {
            writer->err = FLEXI_ERR_BADSTACK;
            return writer->err;
        }

        stack->u.offset = offset;
        stack->key = key;
        stack->type = len == 2   ? FLEXI_TYPE_VECTOR_SINT2
                      : len == 3 ? FLEXI_TYPE_VECTOR_SINT3
                                 : FLEXI_TYPE_VECTOR_SINT4;
        stack->width = stride_bytes;
        return FLEXI_OK;
    } else {
        // Align future writes to the nearest multiple.
        flexi_ssize_t offset;
        if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Length prefix.
        if (!write_uint_by_width(writer, len, stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write values.
        if (!ostream_write(&writer->ostream, ptr, len * stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        flexi_value_s *stack = stack_push(&writer->stack);
        if (stack == NULL) {
            writer->err = FLEXI_ERR_BADSTACK;
            return writer->err;
        }

        stack->u.offset = offset;
        stack->key = key;
        stack->type = FLEXI_TYPE_VECTOR_SINT;
        stack->width = stride_bytes;
        return FLEXI_OK;
    }
}

/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_uint(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, flexi_ssize_t len)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    int stride_bytes = FLEXI_WIDTH_TO_BYTES(stride);
    if (!(stride_bytes >= 1 && stride_bytes <= 8)) {
        return FLEXI_ERR_PARAM;
    }

    if (len >= 2 && len <= 4) {
        // Keep track of the aligned base location of the vector.
        flexi_ssize_t offset;
        if (!write_padding(writer, 0, stride_bytes, &offset)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write values.
        if (!ostream_write(&writer->ostream, ptr, len * stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        flexi_value_s *stack = stack_push(&writer->stack);
        if (stack == NULL) {
            writer->err = FLEXI_ERR_BADSTACK;
            return writer->err;
        }

        stack->u.offset = offset;
        stack->key = key;
        stack->type = len == 2   ? FLEXI_TYPE_VECTOR_UINT2
                      : len == 3 ? FLEXI_TYPE_VECTOR_UINT3
                                 : FLEXI_TYPE_VECTOR_UINT4;
        stack->width = stride_bytes;
        return FLEXI_OK;
    } else {
        // Align future writes to the nearest multiple.
        flexi_ssize_t offset;
        if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Length prefix.
        if (!write_uint_by_width(writer, len, stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write values.
        if (!ostream_write(&writer->ostream, ptr, len * stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        flexi_value_s *stack = stack_push(&writer->stack);
        if (stack == NULL) {
            writer->err = FLEXI_ERR_BADSTACK;
            return writer->err;
        }

        stack->u.offset = offset;
        stack->key = key;
        stack->type = FLEXI_TYPE_VECTOR_UINT;
        stack->width = stride_bytes;
        return FLEXI_OK;
    }
}
/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_flt(flexi_writer_s *writer, const char *key,
    const void *ptr, flexi_width_e stride, flexi_ssize_t len)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    int stride_bytes = FLEXI_WIDTH_TO_BYTES(stride);
    if (stride_bytes != 4 && stride_bytes != 8) {
        return FLEXI_ERR_PARAM;
    }

    if (len >= 2 && len <= 4) {
        // Keep track of the aligned base location of the vector.
        flexi_ssize_t offset;
        if (!write_padding(writer, 0, stride_bytes, &offset)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write values.
        if (!ostream_write(&writer->ostream, ptr, len * stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        flexi_value_s *stack = stack_push(&writer->stack);
        if (stack == NULL) {
            writer->err = FLEXI_ERR_BADSTACK;
            return writer->err;
        }

        stack->u.offset = offset;
        stack->key = key;
        stack->type = len == 2   ? FLEXI_TYPE_VECTOR_FLOAT2
                      : len == 3 ? FLEXI_TYPE_VECTOR_FLOAT3
                                 : FLEXI_TYPE_VECTOR_FLOAT4;
        stack->width = stride_bytes;
        return FLEXI_OK;
    } else {
        // Align future writes to the nearest multiple.
        flexi_ssize_t offset;
        if (!write_padding(writer, stride_bytes, stride_bytes, &offset)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Length prefix.
        if (!write_uint_by_width(writer, len, stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write values.
        if (!ostream_write(&writer->ostream, ptr, len * stride_bytes)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        flexi_value_s *stack = stack_push(&writer->stack);
        if (stack == NULL) {
            writer->err = FLEXI_ERR_BADSTACK;
            return writer->err;
        }

        stack->u.offset = offset;
        stack->key = key;
        stack->type = FLEXI_TYPE_VECTOR_FLOAT;
        stack->width = stride_bytes;
        return FLEXI_OK;
    }
}

/******************************************************************************/

flexi_result_e
flexi_write_blob(flexi_writer_s *writer, const char *key, const void *ptr,
    flexi_ssize_t len, int align)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    int len_width = UINT_WIDTH(len);

    // Pad the blob to the alignment value.
    flexi_ssize_t offset;
    if (!write_padding(writer, len_width, align, &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write the blob length to stream.
    if (!write_uint_by_width(writer, len, len_width)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write the blob.
    if (!ostream_write(&writer->ostream, ptr, len)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Push offset to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_BLOB;
    stack->width = len_width;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_bool(flexi_writer_s *writer, const char *key, bool v)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Push value to the stack.
    flexi_value_s *stack = stack_push(&writer->stack);
    if (!stack) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.u64 = v;
    stack->key = key;
    stack->type = FLEXI_TYPE_BOOL;
    stack->width = 1;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_typed_vector_bool(flexi_writer_s *writer, const char *key,
    const bool *ptr, flexi_ssize_t len)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    // Length prefix.
    if (!write_uint_by_width(writer, len, sizeof(bool))) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Keep track of the base location of the vector.
    flexi_ssize_t offset;
    if (!ostream_tell(&writer->ostream, &offset)) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    // Write values.
    if (!ostream_write(&writer->ostream, ptr, len * sizeof(bool))) {
        writer->err = FLEXI_ERR_BADWRITE;
        return writer->err;
    }

    flexi_value_s *stack = stack_push(&writer->stack);
    if (stack == NULL) {
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    stack->u.offset = offset;
    stack->key = key;
    stack->type = FLEXI_TYPE_VECTOR_BOOL;
    stack->width = sizeof(bool);
    return FLEXI_OK;
}

/******************************************************************************/

flexi_result_e
flexi_write_finalize(flexi_writer_s *writer)
{
    if (FLEXI_ERROR(writer->err)) {
        return FLEXI_ERR_FAILSAFE;
    }

    flexi_value_s *root = writer_peek_idx(writer, 1, 0);
    if (root == NULL) {
        // We have nothing on the stack to write.
        writer->err = FLEXI_ERR_BADSTACK;
        return writer->err;
    }

    if (type_is_direct(root->type)) {
        switch (root->type) {
        case FLEXI_TYPE_NULL: {
            // Write out the null root type.
            const uint8_t buffer[3] = {0x00, 0x00, 0x01};
            if (!ostream_write(&writer->ostream, buffer, COUNTOF(buffer))) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }
            return FLEXI_OK;
        }
        case FLEXI_TYPE_SINT: {
            // Write the number.
            if (!write_sint_by_width(writer, root->u.s64, root->width)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            // Write the type.
            flexi_packed_t type =
                PACK_TYPE(root->type) | PACK_WIDTH(root->width);
            if (!ostream_write(&writer->ostream, &type, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            // Write the width.
            if (!ostream_write(&writer->ostream, &root->width, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            return FLEXI_OK;
        }
        case FLEXI_TYPE_UINT: {
            // Write the number.
            if (!write_uint_by_width(writer, root->u.u64, root->width)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            // Write the type.
            flexi_packed_t type =
                PACK_TYPE(root->type) | PACK_WIDTH(root->width);
            if (!ostream_write(&writer->ostream, &type, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            // Write the width.
            if (!ostream_write(&writer->ostream, &root->width, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            return FLEXI_OK;
        }
        case FLEXI_TYPE_FLOAT: {
            // Write the number.
            switch (root->width) {
            case 4:
                if (!write_f32(writer, root->u.f32, root->width)) {
                    writer->err = FLEXI_ERR_BADWRITE;
                    return writer->err;
                }
                break;
            case 8:
                if (!write_f64(writer, root->u.f64, root->width)) {
                    writer->err = FLEXI_ERR_BADWRITE;
                    return writer->err;
                }
                break;
            default: return FLEXI_ERR_INTERNAL;
            }

            // Write the type.
            flexi_packed_t type =
                PACK_TYPE(root->type) | PACK_WIDTH(root->width);
            if (!ostream_write(&writer->ostream, &type, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            // Write the width.
            if (!ostream_write(&writer->ostream, &root->width, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            return FLEXI_OK;
        }
        case FLEXI_TYPE_BOOL: {
            // Write the boolean.
            bool value = root->u.u64 ? 1 : 0;
            if (!ostream_write(&writer->ostream, &value, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            // Write the type.
            flexi_packed_t type = PACK_TYPE(root->type) | FLEXI_WIDTH_1B;
            if (!ostream_write(&writer->ostream, &type, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            // Write the width.
            if (!ostream_write(&writer->ostream, &root->width, 1)) {
                writer->err = FLEXI_ERR_BADWRITE;
                return writer->err;
            }

            return FLEXI_OK;
        }
        default: return FLEXI_ERR_INTERNAL;
        }
    } else if (type_is_indirect(root->type)) {
        // Get the current location.
        flexi_ssize_t current;
        if (!ostream_tell(&writer->ostream, &current)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Create the offset and figure out its witdh.
        flexi_ssize_t offset = current - root->u.offset;
        int width = UINT_WIDTH(offset);
        if (!write_uint_by_width(writer, offset, width)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write the type that the offset is pointing to.
        flexi_packed_t type = PACK_TYPE(root->type) | PACK_WIDTH(root->width);
        if (!write_uint_by_width(writer, type, 1)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        // Write the width of the offset.
        if (!write_sint_by_width(writer, width, 1)) {
            writer->err = FLEXI_ERR_BADWRITE;
            return writer->err;
        }

        if (stack_pop(&writer->stack, 1) != 1) {
            writer->err = FLEXI_ERR_BADSTACK;
            return writer->err;
        }

        return FLEXI_OK;
    }

    return FLEXI_ERR_INTERNAL;
}

/******************************************************************************/

flexi_result_e
flexi_writer_debug_stack_at(const flexi_writer_s *writer, flexi_ssize_t offset,
    flexi_value_s **value)
{
    flexi_value_s *v = stack_at(&writer->stack, offset);
    if (v == NULL) {
        return FLEXI_ERR_BADSTACK;
    }

    *value = v;
    return FLEXI_OK;
}

/******************************************************************************/

flexi_ssize_t
flexi_writer_debug_stack_count(flexi_writer_s *writer)
{
    return stack_count(&writer->stack);
}
