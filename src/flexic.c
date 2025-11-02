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

#if !defined(NDEBUG) && defined(_MSC_VER)
#define ASSERT(cond) (cond) ? (void)0 : (__debugbreak(), (void)0)
#elif !defined(NDEBUG) && defined(__GNUC__)
#define ASSERT(cond) (cond) ? (void)0 : (__builtin_trap(), (void)0)
#else
#define ASSERT(cond) (void)0
#endif

/******************************************************************************/

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define COUNTOF(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SINT_WIDTH(v) \
    ((v) <= INT8_MAX && (v) >= INT8_MIN      ? 1 \
     : (v) <= INT16_MAX && (v) >= INT16_MIN  ? 2 \
     : (v) <= UINT32_MAX && (v) >= INT32_MIN ? 4 \
                                             : 8);
#define UINT_WIDTH(v) \
    ((v) <= UINT8_MAX ? 1 : (v) <= UINT16_MAX ? 2 : (v) <= UINT32_MAX ? 4 : 8);

#define UNPACK_TYPE(packed) ((flexi_type_e)(packed >> 2))
#define UNPACK_WIDTH(packed) ((flexi_width_e)(packed & 0x03))
#define WIDTH_TO_BYTES(width) ((int)(1 << width))

#define PACK_TYPE(t) ((flexi_packed_t)(t) << 2)
#define PACK_WIDTH(w) \
    ((w) == 1   ? (flexi_packed_t)(0) \
     : (w) == 2 ? (flexi_packed_t)(1) \
     : (w) == 4 ? (flexi_packed_t)(2) \
                : (flexi_packed_t)(3))

typedef enum {
    WRITE_VECTOR_ITER,
    WRITE_VECTOR_ITER_FOR_MAP,
} write_vector_iter_e;

/**
 * @brief Return true if the type is any signed integer type.
 */
static bool type_is_sint(flexi_type_e type) {
    if (type == FLEXI_TYPE_SINT || type == FLEXI_TYPE_INDIRECT_SINT) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type is any unsigned integer type.
 */
static bool type_is_uint(flexi_type_e type) {
    if (type == FLEXI_TYPE_UINT || type == FLEXI_TYPE_INDIRECT_UINT) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type is any int type.
 */
static bool type_is_anyint(flexi_type_e type) {
    switch (type) {
    case FLEXI_TYPE_SINT:
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_SINT:
    case FLEXI_TYPE_INDIRECT_UINT:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Return true if the type is any float type.
 */
static bool type_is_float(flexi_type_e type) {
    if (type == FLEXI_TYPE_FLOAT || type == FLEXI_TYPE_INDIRECT_FLOAT) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type can be written directly, without an offset.
 */
static bool type_is_direct(flexi_type_e type) {
    if (type >= FLEXI_TYPE_NULL && type <= FLEXI_TYPE_FLOAT) {
        return true;
    } else if (type == FLEXI_TYPE_BOOL) {
        return true;
    }
    return false;
}

/**
 * @brief Return true if the type is a vector type.  Types that are vector-like
 *        don't count.
 */
static bool type_is_vector(flexi_type_e type) {
    if (type >= FLEXI_TYPE_VECTOR && type <= FLEXI_TYPE_VECTOR_FLOAT4) {
        return true;
    } else if (type == FLEXI_TYPE_VECTOR_BOOL) {
        return true;
    }
    return false;
}

/**
 * @brief Read a signed integer from the passed buffer.
 *
 * @note Values of width < 8 bytes will be promoted to int64_t.
 *
 * @param dst Output value.
 * @param src Pointer to data to read from.
 * @param width Width of data to read.
 * @return True if read was successful.
 */
static bool read_sint_by_width(int64_t *dst, const char *src, int width) {
    ASSERT(width == 1 || width == 2 || width == 4 || width == 8);

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
 * @brief Write exacty len bytes and return true if the write was complete.
 *
 * @param writer Writer to write to.
 * @param ptr Pointer to data to write.
 * @param len Length of data to write.
 * @return True if write was complete.
 */
static bool writer_write(flexi_writer_s *writer, const void *ptr, size_t len) {
    return writer->write_func(ptr, len, writer->user);
}

/**
 * @brief Return a pointer to already-written data, by absolute offset.
 *
 * @param writer Writer to query.
 * @param offset Offset to find.
 * @return Pointer to data at offset.
 */
static const void *writer_data_at(flexi_writer_s *writer, size_t offset) {
    return writer->data_at_func(offset, writer->user);
}

/**
 * @brief Return the position of the stream.
 *
 * @param writer Writer to query.
 * @param offset Position of the stream.
 * @return True if stream was in a valid state and returned an offset.
 */
static bool writer_tell(flexi_writer_s *writer, size_t *offset) {
    return writer->tell_func(offset, writer->user);
}

/**
 * @brief Write a signed integer to the passed writer.
 *
 * @note Values will first be converted to the equivalent integer of the
 *       passed width.  Values that are too large or small for the passed
 *       width will not be truncated, but will return a failure state.
 *
 * @param writer Writer to operate on.
 * @param v Value to write.
 * @param width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool write_sint_by_width(flexi_writer_s *writer, int64_t v, int width) {
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
    case 8:
        return writer_write(writer, &v, sizeof(int64_t));
    }
    return false;
}

/**
 * @brief Read an unsigned integer from the passed buffer.
 *
 * @note Values of width < 8 bytes will be promoted to uint64_t.
 *
 * @param dst Output value.
 * @param src Pointer to data to read from.
 * @param width Width of data to read.
 * @return True if read was successful.
 */
static bool read_uint_by_width(uint64_t *dst, const char *src, int width) {
    ASSERT(width == 1 || width == 2 || width == 4 || width == 8);

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
 * @brief Write an unsigned integer to the passed writer.
 *
 * @note Values will first be converted to the equivalent integer of the
 *       passed width.  Values that are too large or small for the passed
 *       width will not be truncated, but will return a failure state.
 *
 * @param writer Writer to operate on.
 * @param v Value to write.
 * @param width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool write_uint_by_width(flexi_writer_s *writer, uint64_t v, int width) {
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
    case 8:
        return writer_write(writer, &v, sizeof(uint64_t));
    }
    return false;
}

/**
 * @brief Read a 4-byte float from the passed buffer.
 *
 * @note Floats of width != 4 bytes will be converted.
 *
 * @param dst Output value.
 * @param src Pointer to data to read from.
 * @param width Width of data to read.
 * @return True if read was successful.
 */
static bool read_f32(float *dst, const char *src, int width) {
    ASSERT(width == 4 || width == 8);

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
 * @brief Write a 4-byte float to the passed writer.
 *
 * @note Floats of width != 4 bytes will be converted.
 *
 * @param writer Writer to operate on.
 * @param v Value to write.
 * @param width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool write_f32(flexi_writer_s *writer, float v, int width) {
    ASSERT(width == 4 || width == 8);

    switch (width) {
    case 4:
        return writer_write(writer, &v, sizeof(float));
    case 8: {
        double vv = v;
        return writer_write(writer, &vv, sizeof(double));
    }
    }
    return false;
}

/**
 * @brief Read an 8-byte float from the passed buffer.
 *
 * @note Floats of width != 8 bytes will be converted.
 *
 * @param dst Output value.
 * @param src Pointer to data to read from.
 * @param width Width of data to read.
 * @return True if read was successful.
 */
static bool read_f64(double *dst, const char *src, int width) {
    ASSERT(width == 4 || width == 8);

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
 * @brief Write an 8-byte float to the passed writer.
 *
 * @note Floats of width != 8 bytes will be converted.
 *
 * @param writer Writer to operate on.
 * @param v Value to write.
 * @param width Number of bytes to write.
 * @return True if value was fully written to the writer.
 */
static bool write_f64(flexi_writer_s *writer, double v, int width) {
    ASSERT(width == 4 || width == 8);

    switch (width) {
    case 4: {
        float vv = (float)v;
        return writer_write(writer, &vv, sizeof(float));
    }
    case 8:
        return writer_write(writer, &v, sizeof(double));
    }
    return false;
}

/**
 * @brief Safely resolve an offset value to a new position in the buffer.
 *
 * @param dest Output destination value.
 * @param buffer Buffer to constrain offset resolution.
 * @param src Starting point for offset.
 * @param offset Extent of offset in the negative direction.
 * @return True if offset was resolved.
 */
static bool cursor_resolve_offset(const char **dest,
                                  const flexi_buffer_s *buffer, const char *src,
                                  uint64_t offset) {
    if (offset > PTRDIFF_MAX) {
        return false;
    }

    ptrdiff_t move = (ptrdiff_t)offset;
    ptrdiff_t max_move = src - buffer->data;
    if (move > max_move) {
        return false;
    }

    *dest = src - move;
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a vector or vector-like
 *        type, obtain its length.
 *
 * @param cursor Cursor to check.
 * @param len Length of vector.
 * @return True if length was found.
 */
static bool cursor_get_len(const flexi_cursor_s *cursor, size_t *len) {
    ASSERT(cursor->type == FLEXI_TYPE_STRING ||
           cursor->type == FLEXI_TYPE_MAP ||
           cursor->type == FLEXI_TYPE_VECTOR ||
           cursor->type == FLEXI_TYPE_VECTOR_SINT ||
           cursor->type == FLEXI_TYPE_VECTOR_UINT ||
           cursor->type == FLEXI_TYPE_VECTOR_FLOAT ||
           cursor->type == FLEXI_TYPE_BLOB ||
           cursor->type == FLEXI_TYPE_VECTOR_BOOL);

    uint64_t v;
    if (!read_uint_by_width(&v, cursor->cursor - cursor->stride,
                            cursor->stride)) {
        return false;
    }

    *len = (size_t)v;
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a vector which contains
 *        trailing types, return a pointer to the first type.
 *
 * @param cursor Cursor to check.
 * @param packed Pointer to first packed type.
 * @return True if types were located.
 */
static bool cursor_vector_types(const flexi_cursor_s *cursor,
                                const flexi_packed_t **packed) {
    ASSERT(cursor->type == FLEXI_TYPE_MAP || cursor->type == FLEXI_TYPE_VECTOR);

    size_t len = 0;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    const flexi_packed_t *p = (const flexi_packed_t *)cursor->cursor;
    *packed = p + len * cursor->stride;
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a vector which contains
 *        trailing types, return a cursor pointing at the value at the given
 *        index.
 *
 * @param cursor Cursor to use as base.
 * @param index Index to look up.
 * @param dest Destination cursor.
 * @return True if seek was successful.
 */
static bool cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index,
                                     flexi_cursor_s *dest) {
    ASSERT(cursor->type == FLEXI_TYPE_MAP || cursor->type == FLEXI_TYPE_VECTOR);

    const flexi_packed_t *types = NULL;
    if (!cursor_vector_types(cursor, &types)) {
        return false;
    }

    flexi_type_e type = UNPACK_TYPE(types[index]);
    if (type_is_direct(type)) {
        // No need to resolve an offset, we're pretty much done.
        dest->buffer = cursor->buffer;
        dest->cursor = cursor->cursor + (index * (size_t)cursor->stride);
        dest->type = type;
        dest->stride = cursor->stride;
        return true;
    }

    uint64_t offset = 0;
    const char *offset_ptr = cursor->cursor + (index * (size_t)cursor->stride);
    if (!read_uint_by_width(&offset, offset_ptr, cursor->stride)) {
        return false;
    }

    if (!cursor_resolve_offset(&dest->cursor, cursor->buffer, offset_ptr,
                               offset)) {
        return false;
    }

    dest->buffer = cursor->buffer;
    dest->type = type;
    dest->stride = WIDTH_TO_BYTES(UNPACK_WIDTH(types[index]));
    return true;
}

/**
 * @brief Given a cursor pointing at the base of a map, return the map key
 *        used by that index.
 *
 * @param cursor Cursor to use as base.
 * @param index Index to look up.
 * @param str Key found by lookup.
 * @return True if lookup was successful.
 */
static bool cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index,
                                    const char **str) {
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    // Figure out offset and width of keys vector.
    uint64_t keys_offset, keys_width;
    if (!read_uint_by_width(&keys_offset, cursor->cursor - (cursor->stride * 3),
                            cursor->stride)) {
        return false;
    }

    if (!read_uint_by_width(&keys_width, cursor->cursor - (cursor->stride * 2),
                            cursor->stride)) {
        return false;
    }

    // Seek the keys base.
    const char *keys_base =
        cursor->cursor - ((3 * cursor->stride) + keys_offset);

    // Resolve the key.
    uint64_t offset;
    const char *offset_ptr = keys_base + (index * (size_t)keys_width);
    if (!read_uint_by_width(&offset, offset_ptr, (int)keys_width)) {
        return false;
    }

    return cursor_resolve_offset(str, cursor->buffer, offset_ptr, offset);
}

/**
 * @brief Seek a map key via linear search.
 *
 * @param cursor Cursor of map to use as base.
 * @param len Length of map at cursor.
 * @param key Key to check for.
 * @param dest Destination cursor.
 * @return True if lookup was successful.
 */
static bool cursor_seek_map_key_linear(const flexi_cursor_s *cursor, size_t len,
                                       const char *key, flexi_cursor_s *dest) {
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    // Linear search.
    for (size_t i = 0; i < len; i++) {
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, i, &cmp)) {
            return false;
        }

        if (!strcmp(cmp, key)) {
            return cursor_seek_vector_index(cursor, i, dest);
        }
    }

    return false;
}

/**
 * @brief Seek a map key via binary search.
 *
 * @param cursor Cursor of map to use as base.
 * @param len Length of map at cursor.
 * @param key Key to check for.
 * @param dest Destination cursor.
 * @return True if lookup was successful.
 */
static bool cursor_seek_map_key_bsearch(const flexi_cursor_s *cursor,
                                        size_t len, const char *key,
                                        flexi_cursor_s *dest) {
    ASSERT(cursor->type == FLEXI_TYPE_MAP);

    size_t left = 0;
    size_t right = len - 1;
    while (left <= right) {
        size_t i = left + ((right - left) / 2);
        const char *cmp = NULL;
        if (!cursor_map_key_at_index(cursor, i, &cmp)) {
            return false;
        }

        int res = strcmp(cmp, key);
        if (res == 0) {
            return cursor_seek_vector_index(cursor, i, dest);
        }

        if (res < 0) {
            left = i + 1;
        } else {
            right = i - 1;
        }
    }

    return false;
}

/******************************************************************************/

static bool reader_emit_sint(const flexi_reader_s *reader,
                             const flexi_cursor_s *cursor) {
    int64_t u;
    if (!read_sint_by_width(&u, cursor->cursor, cursor->stride)) {
        return false;
    }

    reader->sint(u);
    return true;
}

/******************************************************************************/

static bool reader_emit_uint(const flexi_reader_s *reader,
                             const flexi_cursor_s *cursor) {
    uint64_t u;
    if (!read_uint_by_width(&u, cursor->cursor, cursor->stride)) {
        return false;
    }

    reader->uint(u);
    return true;
}

/******************************************************************************/

static bool reader_emit_float(const flexi_reader_s *reader,
                              const flexi_cursor_s *cursor) {
    switch (cursor->stride) {
    case 4: {
        float u;
        if (!read_f32(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        reader->f32(u);
        return true;
    }
    case 8: {
        double u;
        if (!read_f64(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        reader->f64(u);
        return true;
    }
    }
    return false;
}

/******************************************************************************/

static bool reader_emit_string(const flexi_reader_s *reader,
                               const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->string(cursor->cursor, len);
    return true;
}

/******************************************************************************/

static bool reader_emit_map(const flexi_reader_s *reader,
                            const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->map_begin(len);
    for (size_t i = 0; i < len; i++) {
        const char *key;
        if (!cursor_map_key_at_index(cursor, i, &key)) {
            return false;
        }

        reader->map_key(key);

        flexi_cursor_s value;
        if (!cursor_seek_vector_index(cursor, i, &value)) {
            return false;
        }
        if (!flexi_read(reader, &value)) {
            return false;
        }
    }

    reader->map_end();
    return true;
}

/******************************************************************************/

static bool reader_emit_vector(const flexi_reader_s *reader,
                               const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->vector_begin(len);
    for (size_t i = 0; i < len; i++) {
        flexi_cursor_s value;
        if (!cursor_seek_vector_index(cursor, i, &value)) {
            return false;
        }
        if (!flexi_read(reader, &value)) {
            return false;
        }
    }

    reader->vector_end();
    return true;
}

/******************************************************************************/

static bool reader_emit_vector_sint(const flexi_reader_s *reader,
                                    const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->typed_vector_sint(cursor->cursor, cursor->stride, len);
    return true;
}

/******************************************************************************/

static bool reader_emit_vector_uint(const flexi_reader_s *reader,
                                    const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->typed_vector_uint(cursor->cursor, cursor->stride, len);
    return true;
}

/******************************************************************************/

static bool reader_emit_vector_float(const flexi_reader_s *reader,
                                     const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->typed_vector_flt(cursor->cursor, cursor->stride, len);
    return true;
}

/******************************************************************************/

static bool reader_emit_vector_key(const flexi_reader_s *reader,
                                   const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->vector_begin(len);
    for (size_t i = 0; i < len; i++) {
        // Access the offset by hand.
        uint64_t offset = 0;
        const char *offset_ptr = cursor->cursor + (i * (size_t)cursor->stride);
        if (!read_uint_by_width(&offset, offset_ptr, cursor->stride)) {
            return false;
        }

        const char *dest = NULL;
        if (!cursor_resolve_offset(&dest, cursor->buffer, offset_ptr, offset)) {
            return false;
        }

        reader->key(dest);
    }

    reader->vector_end();
    return true;
}

/******************************************************************************/

static bool reader_emit_vector_blob(const flexi_reader_s *reader,
                                    const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->blob(cursor->cursor, len);
    return true;
}

/******************************************************************************/

static bool reader_emit_vector_bool(const flexi_reader_s *reader,
                                    const flexi_cursor_s *cursor) {
    size_t len;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    reader->typed_vector_bool((const bool *)cursor->cursor, len);
    return true;
}

/******************************************************************************/

static flexi_value_s *writer_push(flexi_writer_s *writer) {
    if (writer->head >= (int)COUNTOF(writer->stack)) {
        return NULL;
    }

    flexi_value_s *value = &writer->stack[writer->head];
    writer->head += 1;
    return value;
}

/******************************************************************************/

static flexi_value_s *writer_peek(flexi_writer_s *writer, int offset) {
    flexi_value_s *value = &writer->stack[writer->head + offset];
    return value;
}

/******************************************************************************/

static bool writer_pop(flexi_writer_s *writer, int count) {
    writer->head -= count;
    return true;
}

/**
 * @brief Obtain the key for an inline map with interleaved keys and values.
 *
 * @param writer Writer to operate on.
 * @param start Starting index in the stack for the map.
 * @param index Array-based index for the map.
 * @return Pointer to key off the stack.
 */
static flexi_value_s *writer_get_inline_key(flexi_writer_s *writer, int start,
                                            int index) {
    int v_index = start + (index * 2);
    ASSERT(v_index >= 0 && v_index < writer->head);
    flexi_value_s *v = &writer->stack[v_index];
    ASSERT(v->type == FLEXI_TYPE_KEY);
    return v;
}

/**
 * @brief Sort the interleved keys and values of a map n so all keys are
 *        in strcmp order.
 *
 * @param writer Writer to operate on.
 * @param len Number of keys and values to use for map - must be 1/2 the
 *            number of pushed keys and values.
 * @return True if sort was successful.
 */
static bool writer_sort_inline_map(flexi_writer_s *writer, size_t len) {
    int start = writer->head - ((int)len * 2);
    ASSERT(start >= 0);

    // TODO: Use a faster sort for large key sets.
    for (int i = 1; i < (int)len; i++) {
        flexi_value_s *cur = writer_get_inline_key(writer, start, i);
        size_t curoff = cur->u.offset;
        const char *curkey = writer_data_at(writer, curoff);
        flexi_value_s curvalue = *(cur + 1);

        int j = i;
        for (; j > 0; j--) {
            flexi_value_s *seek = writer_get_inline_key(writer, start, j - 1);
            const char *seekkey = writer_data_at(writer, seek->u.offset);

            int cmp = strcmp(curkey, seekkey);
            if (cmp > 0) {
                break;
            }

            flexi_value_s *dest = writer_get_inline_key(writer, start, j);
            dest->u.offset = seek->u.offset;
            memcpy(dest + 1, seek + 1, sizeof(flexi_value_s));
        }

        if (i != j) {
            flexi_value_s *dest = writer_get_inline_key(writer, start, j);
            dest->u.offset = curoff;
            memcpy(dest + 1, &curvalue, sizeof(flexi_value_s));
        }
    }

    return true;
}

/******************************************************************************/

static bool write_vector_values(flexi_writer_s *writer, size_t len, int stride,
                                write_vector_iter_e iter) {
    // FIXME: Validate these calculations.
    int i, iter_stride;
    switch (iter) {
    case WRITE_VECTOR_ITER:
        i = 0 - (int)len;
        iter_stride = 1;
        break;
    case WRITE_VECTOR_ITER_FOR_MAP:
        i = 1 - (int)len * 2;
        iter_stride = 2;
        break;
    default:
        return false;
    }

    // Write values
    for (; i < 0; i += iter_stride) {
        const flexi_value_s *value = writer_peek(writer, i);
        switch (value->type) {
        case FLEXI_TYPE_SINT:
            // Write value inline.
            if (!write_sint_by_width(writer, value->u.s64, stride)) {
                return false;
            }
            break;
        case FLEXI_TYPE_UINT:
            // Write value inline.
            if (!write_uint_by_width(writer, value->u.u64, stride)) {
                return false;
            }
            break;
        case FLEXI_TYPE_FLOAT:
            // Write value inline.
            switch (value->width) {
            case 4:
                if (!write_f32(writer, value->u.f32, stride)) {
                    return false;
                }
                break;
            case 8:
                if (!write_f64(writer, value->u.f64, stride)) {
                    return false;
                }
                break;
            default:
                return false;
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
                return false;
            }

            // Calculate and write offset.
            size_t offset = current - value->u.offset;
            if (!write_uint_by_width(writer, offset, stride)) {
                return false;
            }
            break;
        }
        case FLEXI_TYPE_BOOL:
            // Write value inline.
            if (!write_uint_by_width(writer, value->u.u64, stride)) {
                return false;
            }
            break;
        default:
            return false;
        }
    }
    return true;
}

/******************************************************************************/

static bool write_vector_types(flexi_writer_s *writer, size_t len) {
    for (int i = 0; i < (int)len; i++) {
        int stack_index = 0 - (int)len + i;
        const flexi_value_s *value = writer_peek(writer, stack_index);
        flexi_packed_t packed =
            PACK_TYPE(value->type) | PACK_WIDTH(value->width);
        if (!writer_write(writer, &packed, sizeof(flexi_packed_t))) {
            return false;
        }
    }

    return true;
}

/******************************************************************************/

flexi_buffer_s flexi_make_buffer(const void *buffer, size_t len) {
    flexi_buffer_s rvo;
    rvo.data = (const char *)buffer;
    rvo.length = (ptrdiff_t)len;
    return rvo;
}

/******************************************************************************/

bool flexi_buffer_open(const flexi_buffer_s *buffer, flexi_cursor_s *cursor) {
    if (buffer->length < 3 || buffer->length > PTRDIFF_MAX) {
        // Shortest length we can discard without checking.
        return false;
    }

    // Width of root object.
    cursor->buffer = buffer;
    cursor->cursor = buffer->data + buffer->length - 1;
    uint8_t root_bytes = *(const uint8_t *)(cursor->cursor);
    if (buffer->length < root_bytes + 2) {
        return false;
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
        cursor->stride = root_bytes;
        return true;
    }

    // We're pointing at an offset, resolve it.
    uint64_t offset;
    if (!read_uint_by_width(&offset, cursor->cursor, root_bytes)) {
        return false;
    }

    const char *dest = NULL;
    if (!cursor_resolve_offset(&dest, cursor->buffer, cursor->cursor, offset)) {
        return false;
    }

    cursor->cursor = dest;
    cursor->type = type;
    cursor->stride = WIDTH_TO_BYTES(UNPACK_WIDTH(packed));
    return true;
}

/******************************************************************************/

flexi_type_e flexi_cursor_type(const flexi_cursor_s *cursor) {
    return cursor->type;
}

/******************************************************************************/

int flexi_cursor_stride(const flexi_cursor_s *cursor) { return cursor->stride; }

/******************************************************************************/

bool flexi_cursor_length(const flexi_cursor_s *cursor, size_t *len) {
    switch (cursor->type) {
    case FLEXI_TYPE_STRING:
    case FLEXI_TYPE_MAP:
    case FLEXI_TYPE_VECTOR:
    case FLEXI_TYPE_VECTOR_SINT:
    case FLEXI_TYPE_VECTOR_UINT:
    case FLEXI_TYPE_VECTOR_FLOAT:
    case FLEXI_TYPE_BLOB:
    case FLEXI_TYPE_VECTOR_BOOL:
        return cursor_get_len(cursor, len);
    case FLEXI_TYPE_VECTOR_SINT2:
    case FLEXI_TYPE_VECTOR_UINT2:
    case FLEXI_TYPE_VECTOR_FLOAT2:
        *len = 2;
        return true;
    case FLEXI_TYPE_VECTOR_SINT3:
    case FLEXI_TYPE_VECTOR_UINT3:
    case FLEXI_TYPE_VECTOR_FLOAT3:
        *len = 3;
        return true;
    case FLEXI_TYPE_VECTOR_SINT4:
    case FLEXI_TYPE_VECTOR_UINT4:
    case FLEXI_TYPE_VECTOR_FLOAT4:
        *len = 4;
        return true;
    default:
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_bool(const flexi_cursor_s *cursor, bool *v) {
    if (type_is_anyint(cursor->type)) {
        uint64_t u;
        if (!read_uint_by_width(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (bool)u;
        return true;
    } else if (cursor->type == FLEXI_TYPE_FLOAT &&
               cursor->stride == sizeof(float)) {
        float u;
        if (!read_f32(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (bool)u;
        return true;
    } else if (cursor->type == FLEXI_TYPE_FLOAT &&
               cursor->stride == sizeof(double)) {
        double u;
        if (!read_f64(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (bool)u;
        return true;
    } else if (cursor->type == FLEXI_TYPE_BOOL) {
        *v = *(const bool *)(cursor->cursor);
        return true;
    } else {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_sint(const flexi_cursor_s *cursor, int64_t *v) {
    if (type_is_anyint(cursor->type)) {
        int64_t u;
        if (!read_sint_by_width(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 4) {
        float u;
        if (!read_f32(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (int64_t)u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 8) {
        double u;
        if (!read_f64(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (int64_t)u;
        return true;
    } else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1) {
        *v = *(const bool *)(cursor->cursor);
        return true;
    } else {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_uint(const flexi_cursor_s *cursor, uint64_t *v) {
    if (type_is_anyint(cursor->type)) {
        uint64_t u;
        if (!read_uint_by_width(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 4) {
        float u;
        if (!read_f32(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (uint64_t)u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 8) {
        double u;
        if (!read_f64(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (uint64_t)u;
        return true;
    }

    else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1) {
        *v = *(const bool *)(cursor->cursor);
        return true;
    } else {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_f32(const flexi_cursor_s *cursor, float *v) {
    if (type_is_sint(cursor->type)) {
        int64_t u;
        if (!read_sint_by_width(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (float)u;
        return true;
    } else if (type_is_uint(cursor->type)) {
        uint64_t u;
        if (!read_uint_by_width(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (float)u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 4) {
        float u;
        if (!read_f32(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 8) {
        double u;
        if (!read_f64(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (float)u;
        return true;
    } else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1) {
        *v = (float)*(const bool *)(cursor->cursor);
        return true;
    } else {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_f64(const flexi_cursor_s *cursor, double *v) {
    if (type_is_sint(cursor->type)) {
        int64_t u;
        if (!read_sint_by_width(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (double)u;
        return true;
    } else if (type_is_uint(cursor->type)) {
        uint64_t u;
        if (!read_uint_by_width(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = (double)u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 4) {
        float u;
        if (!read_f32(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = u;
        return true;
    } else if (type_is_float(cursor->type) && cursor->stride == 8) {
        double u;
        if (!read_f64(&u, cursor->cursor, cursor->stride)) {
            return false;
        }

        *v = u;
        return true;
    } else if (cursor->type == FLEXI_TYPE_BOOL && cursor->stride == 1) {
        *v = (double)*(const bool *)(cursor->cursor);
        return true;
    } else {
        return false;
    }
}

/******************************************************************************/

bool flexi_cursor_string(const flexi_cursor_s *cursor, const char **str) {
    switch (cursor->type) {
    case FLEXI_TYPE_STRING:
        break;
    case FLEXI_TYPE_KEY:
        break;
    default:
        return false;
    }

    *str = cursor->cursor;
    return true;
}

/******************************************************************************/

bool flexi_cursor_key(const flexi_cursor_s *cursor, const char **str) {
    switch (cursor->type) {
    case FLEXI_TYPE_STRING:
        break;
    case FLEXI_TYPE_KEY:
        break;
    default:
        return false;
    }

    *str = cursor->cursor;
    return true;
}

/******************************************************************************/

bool flexi_cursor_blob(const flexi_cursor_s *cursor, const uint8_t **blob) {
    if (cursor->type != FLEXI_TYPE_BLOB) {
        return false;
    }

    *blob = (const uint8_t *)cursor->cursor;
    return true;
}

/******************************************************************************/

bool flexi_cursor_typed_vector_data(const flexi_cursor_s *cursor,
                                    const void **data) {
    if (cursor->type == FLEXI_TYPE_VECTOR || !type_is_vector(cursor->type)) {
        return false;
    }

    *data = cursor->cursor;
    return true;
}

/******************************************************************************/

bool flexi_cursor_vector_types(const flexi_cursor_s *cursor,
                               const flexi_packed_t **packed) {
    if (cursor->type != FLEXI_TYPE_VECTOR && cursor->type != FLEXI_TYPE_MAP) {
        return false;
    }

    return cursor_vector_types(cursor, packed);
}

/******************************************************************************/

bool flexi_cursor_seek_vector_index(const flexi_cursor_s *cursor, size_t index,
                                    flexi_cursor_s *dest) {
    if (cursor->type != FLEXI_TYPE_VECTOR && cursor->type != FLEXI_TYPE_MAP) {
        return false;
    }

    return cursor_seek_vector_index(cursor, index, dest);
}

/******************************************************************************/

bool flexi_cursor_map_key_at_index(const flexi_cursor_s *cursor, size_t index,
                                   const char **str) {
    if (cursor->type != FLEXI_TYPE_MAP) {
        return false;
    }

    return cursor_map_key_at_index(cursor, index, str);
}

/******************************************************************************/

bool flexi_cursor_seek_map_key(const flexi_cursor_s *cursor, const char *key,
                               flexi_cursor_s *dest) {
    if (cursor->type != FLEXI_TYPE_MAP) {
        return false;
    }

    size_t len = 0;
    if (!cursor_get_len(cursor, &len)) {
        return false;
    }

    if (len <= 16) {
        return cursor_seek_map_key_linear(cursor, len, key, dest);
    } else {
        return cursor_seek_map_key_bsearch(cursor, len, key, dest);
    }
}

/******************************************************************************/

bool flexi_read(const flexi_reader_s *reader, const flexi_cursor_s *cursor) {
    switch (cursor->type) {
    case FLEXI_TYPE_NULL:
        reader->null();
        return true;
    case FLEXI_TYPE_SINT:
    case FLEXI_TYPE_INDIRECT_SINT:
        return reader_emit_sint(reader, cursor);
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_INDIRECT_UINT:
        return reader_emit_uint(reader, cursor);
    case FLEXI_TYPE_FLOAT:
    case FLEXI_TYPE_INDIRECT_FLOAT:
        return reader_emit_float(reader, cursor);
    case FLEXI_TYPE_KEY:
        reader->key(cursor->cursor);
        return true;
    case FLEXI_TYPE_STRING:
        return reader_emit_string(reader, cursor);
    case FLEXI_TYPE_MAP:
        return reader_emit_map(reader, cursor);
    case FLEXI_TYPE_VECTOR:
        return reader_emit_vector(reader, cursor);
    case FLEXI_TYPE_VECTOR_SINT:
        return reader_emit_vector_sint(reader, cursor);
    case FLEXI_TYPE_VECTOR_UINT:
        return reader_emit_vector_uint(reader, cursor);
    case FLEXI_TYPE_VECTOR_FLOAT:
        return reader_emit_vector_float(reader, cursor);
    case FLEXI_TYPE_VECTOR_KEY:
        return reader_emit_vector_key(reader, cursor);
    case FLEXI_TYPE_VECTOR_SINT2:
        reader->typed_vector_sint(cursor->cursor, cursor->stride, 2);
        return true;
    case FLEXI_TYPE_VECTOR_UINT2:
        reader->typed_vector_uint(cursor->cursor, cursor->stride, 2);
        return true;
    case FLEXI_TYPE_VECTOR_FLOAT2:
        reader->typed_vector_flt(cursor->cursor, cursor->stride, 2);
        return true;
    case FLEXI_TYPE_VECTOR_SINT3:
        reader->typed_vector_sint(cursor->cursor, cursor->stride, 3);
        return true;
    case FLEXI_TYPE_VECTOR_UINT3:
        reader->typed_vector_uint(cursor->cursor, cursor->stride, 3);
        return true;
    case FLEXI_TYPE_VECTOR_FLOAT3:
        reader->typed_vector_flt(cursor->cursor, cursor->stride, 3);
        return true;
    case FLEXI_TYPE_VECTOR_SINT4:
        reader->typed_vector_sint(cursor->cursor, cursor->stride, 4);
        return true;
    case FLEXI_TYPE_VECTOR_UINT4:
        reader->typed_vector_uint(cursor->cursor, cursor->stride, 4);
        return true;
    case FLEXI_TYPE_VECTOR_FLOAT4:
        reader->typed_vector_flt(cursor->cursor, cursor->stride, 4);
        return true;
    case FLEXI_TYPE_BLOB:
        return reader_emit_vector_blob(reader, cursor);
    case FLEXI_TYPE_BOOL:
        reader->boolean(*(const bool *)(cursor->cursor));
        return true;
    case FLEXI_TYPE_VECTOR_BOOL:
        return reader_emit_vector_bool(reader, cursor);
    }

    return false;
}

/******************************************************************************/

bool flexi_write_bool(flexi_writer_s *writer, bool v) {
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_BOOL;
    stack->width = 1;
    stack->u.u64 = v;
    return true;
}

/******************************************************************************/

bool flexi_write_sint(flexi_writer_s *writer, int64_t v) {
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_SINT;
    stack->width = SINT_WIDTH(v);
    stack->u.u64 = v;
    return true;
}

/******************************************************************************/

bool flexi_write_uint(flexi_writer_s *writer, uint64_t v) {
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_UINT;
    stack->width = UINT_WIDTH(v);
    stack->u.u64 = v;
    return true;
}

/******************************************************************************/

bool flexi_write_f32(flexi_writer_s *writer, float v) {
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_FLOAT;
    stack->width = sizeof(float);
    stack->u.f32 = v;
    return true;
}

/******************************************************************************/

bool flexi_write_f64(flexi_writer_s *writer, double v) {
    // Push value to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_FLOAT;
    stack->width = sizeof(double);
    stack->u.f64 = v;
    return true;
}

/******************************************************************************/

bool flexi_write_string(flexi_writer_s *writer, const char *str) {
    // Write the string length to stream.
    size_t len = strlen(str);
    int width = UINT_WIDTH(len);
    if (!write_uint_by_width(writer, len, width)) {
        return false;
    }

    // Keep track of string starting position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write the string, plus the trailing '\0'.
    if (!writer_write(writer, str, len + 1)) {
        return false;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_STRING;
    stack->width = width;
    stack->u.offset = offset;
    return true;
}

/******************************************************************************/

bool flexi_write_key(flexi_writer_s *writer, const char *str) {
    // Keep track of string starting position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write the string, plus the trailing '\0'.
    size_t len = strlen(str);
    if (!writer_write(writer, str, len + 1)) {
        return false;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_KEY;
    stack->width = 0;
    stack->u.offset = offset;
    return true;
}

/******************************************************************************/

bool flexi_write_blob(flexi_writer_s *writer, const void *ptr, size_t len) {
    // Write the blob length to stream.
    int width = UINT_WIDTH(len);
    if (!write_uint_by_width(writer, len, width)) {
        return false;
    }

    // Keep track of string starting position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write the blob.
    if (!writer_write(writer, ptr, len)) {
        return false;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_BLOB;
    stack->width = width;
    stack->u.offset = offset;
    return true;
}

/******************************************************************************/

bool flexi_write_indirect_sint(flexi_writer_s *writer, int64_t v) {
    // Keep track of the value's position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write the value to the stream - large enough to fit.
    int width = SINT_WIDTH(v);
    if (!write_sint_by_width(writer, v, width)) {
        return false;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_INDIRECT_SINT;
    stack->width = width;
    stack->u.offset = offset;
    return true;
}

/******************************************************************************/

bool flexi_write_indirect_uint(flexi_writer_s *writer, uint64_t v) {
    // Keep track of the value's position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write the value to the stream - large enough to fit.
    int width = UINT_WIDTH(v);
    if (!write_uint_by_width(writer, v, width)) {
        return false;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_INDIRECT_UINT;
    stack->width = width;
    stack->u.offset = offset;
    return true;
}

/******************************************************************************/

bool flexi_write_indirect_f32(flexi_writer_s *writer, float v) {
    // Keep track of the value's position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_f32(writer, v, sizeof(float))) {
        return false;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_INDIRECT_FLOAT;
    stack->width = sizeof(float);
    stack->u.offset = offset;
    return true;
}

/******************************************************************************/

bool flexi_write_indirect_f64(flexi_writer_s *writer, double v) {
    // Keep track of the value's position.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write the value to the stream - large enough to fit.
    if (!write_f64(writer, v, sizeof(double))) {
        return false;
    }

    // Push offset to the stack.
    flexi_value_s *stack = writer_push(writer);
    if (!stack) {
        return false;
    }

    stack->type = FLEXI_TYPE_INDIRECT_FLOAT;
    stack->width = sizeof(double);
    stack->u.offset = offset;
    return true;
}

/******************************************************************************/

bool flexi_write_inline_map(flexi_writer_s *writer, size_t len,
                            flexi_width_e stride) {
    // Find the start of the keypairs on the stack.
    int start = writer->head - ((int)len * 2);
    if (start < 0) {
        return false;
    }

    // All keys must be of key type.  Avoids redundant checks later.
    for (int i = start; i < writer->head; i += 2) {
        if (writer->stack[i].type != FLEXI_TYPE_KEY) {
            return false;
        }
    }

    // Sort the keys and values by key name.
    if (!writer_sort_inline_map(writer, len)) {
        return false;
    }

    int stride_bytes = WIDTH_TO_BYTES(stride);

    // Find the start of the interleaved keys.
    start = writer->head - ((int)len * 2);
    if (start < 0) {
        return false;
    }

    // Save the base of the keys vector for later.
    size_t keys_offset;
    if (!writer_tell(writer, &keys_offset)) {
        return false;
    }

    // We already have the keys written out, write out the keys vector.
    for (int i = start; i < writer->head; i += 2) {
        flexi_value_s *value = &writer->stack[i];
        ASSERT(value->type == FLEXI_TYPE_KEY);

        // TODO: We can pre-calculate this.
        size_t current;
        if (!writer_tell(writer, &current)) {
            return false;
        }

        size_t offset = current - value->u.offset;
        if (!write_uint_by_width(writer, offset, stride_bytes)) {
            return false;
        }
    }

    // Offset to keys vector.
    size_t current;
    if (!writer_tell(writer, &current)) {
        return false;
    }

    size_t offset = current - keys_offset;
    if (!write_uint_by_width(writer, offset, stride_bytes)) {
        return false;
    }

    // Byte width of key vector.
    if (!write_uint_by_width(writer, stride_bytes, stride_bytes)) {
        return false;
    }

    // Now we're to the values - write length.
    if (!write_uint_by_width(writer, offset, stride_bytes)) {
        return false;
    }

    // Keep track of the base location of the vector.
    size_t values_offset;
    if (!writer_tell(writer, &values_offset)) {
        return false;
    }

    // Write values.
    if (!write_vector_values(writer, len, stride_bytes,
                             WRITE_VECTOR_ITER_FOR_MAP)) {
        return false;
    }

    // Write types.
    if (!write_vector_types(writer, len)) {
        return false;
    }

    writer_pop(writer, (int)len * 2);
    flexi_value_s *stack = writer_push(writer);
    stack->type = FLEXI_TYPE_MAP;
    stack->u.offset = values_offset;
    stack->width = stride_bytes;
    return true;
}

/******************************************************************************/

bool flexi_write_vector(flexi_writer_s *writer, size_t len,
                        flexi_width_e stride) {
    int stride_bytes = WIDTH_TO_BYTES(stride);

    // Write length
    if (!write_uint_by_width(writer, len, stride_bytes)) {
        return false;
    }

    // Keep track of the base location of the vector.
    size_t offset;
    if (!writer_tell(writer, &offset)) {
        return false;
    }

    // Write values.
    if (!write_vector_values(writer, len, stride_bytes, WRITE_VECTOR_ITER)) {
        return false;
    }

    // Write types.
    if (!write_vector_types(writer, len)) {
        return false;
    }

    writer_pop(writer, (int)len);
    flexi_value_s *stack = writer_push(writer);
    stack->type = FLEXI_TYPE_VECTOR;
    stack->u.offset = offset;
    stack->width = stride_bytes;
    return true;
}

/******************************************************************************/

bool flexi_write_finalize(flexi_writer_s *writer) {
    flexi_value_s *root = writer_peek(writer, -1);
    if (root == NULL) {
        // We have nothing on the stack to write.
        return false;
    }

    switch (root->type) {
    case FLEXI_TYPE_NULL:
    case FLEXI_TYPE_SINT:
    case FLEXI_TYPE_UINT:
    case FLEXI_TYPE_FLOAT:
    case FLEXI_TYPE_BOOL:
        return false;
    case FLEXI_TYPE_MAP:
    case FLEXI_TYPE_VECTOR: {
        // Get the current location.
        size_t current;
        if (!writer_tell(writer, &current)) {
            return false;
        }

        // Create the offset and figure out its witdh.
        size_t offset = current - root->u.offset;
        int width = UINT_WIDTH(offset);
        if (!write_uint_by_width(writer, offset, width)) {
            return false;
        }

        // Write the type that the offset is pointing to.
        flexi_packed_t type = PACK_TYPE(root->type) | PACK_WIDTH(root->width);
        if (!write_uint_by_width(writer, type, 1)) {
            return false;
        }

        // Write the width of the offset.
        if (!write_sint_by_width(writer, width, 1)) {
            return false;
        }

        if (!writer_pop(writer, 1)) {
            // How did this happen???
            return false;
        }
        return true;
    }
    default:
        return false;
    }
}
