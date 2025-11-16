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

#include "tests.hpp"

static void
GetCursorInt64Pattern(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 10> s_data = {
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x07, 0x08};

    auto buffer = flexi_make_buffer(s_data.data(), s_data.size());
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
}

static void
GetCursorUint64Pattern(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 10> s_data = {
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x0b, 0x08};

    auto buffer = flexi_make_buffer(s_data.data(), s_data.size());
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
}

TEST(CursorInt, Types_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    ASSERT_EQ(FLEXI_TYPE_SINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_width(&cursor));
    ASSERT_EQ(0, flexi_cursor_length(&cursor));
}

TEST(CursorInt, Sint_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    int64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&cursor, &v));
    ASSERT_EQ(INT64_PATTERN, v);
}

TEST(CursorInt, Uint_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    uint64_t v = 1;
    ASSERT_EQ(FLEXI_ERR_RANGE, flexi_cursor_uint(&cursor, &v));
    ASSERT_EQ(0, v);
}

TEST(CursorInt, Float32_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    constexpr float ex = float(INT64_PATTERN);
    float v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&cursor, &v));
    ASSERT_EQ(ex, v);
}

TEST(CursorInt, Float64_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    constexpr double ex = double(INT64_PATTERN);
    double v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&cursor, &v));
    ASSERT_EQ(ex, v);
}

TEST(CursorInt, Key_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_key(&cursor, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorInt, String_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const char *v = nullptr;
    flexi_ssize_t len = -1;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_string(&cursor, &v, &len));
    ASSERT_STREQ("", v);
    ASSERT_EQ(0, len);
}

TEST(CursorInt, TypedVectorData_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const void *v = nullptr;
    flexi_type_e t = FLEXI_TYPE_NULL;
    int s = -1;
    flexi_ssize_t c = -1;

    ASSERT_EQ(FLEXI_ERR_BADTYPE,
        flexi_cursor_typed_vector_data(&cursor, &v, &t, &s, &c));
    ASSERT_EQ(0, *static_cast<const int64_t *>(v));
    ASSERT_EQ(FLEXI_TYPE_INVALID, t);
    ASSERT_EQ(0, s);
    ASSERT_EQ(0, c);
}

TEST(CursorInt, VectorTypes_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const flexi_packed_t *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_vector_types(&cursor, &v));
    ASSERT_EQ(FLEXI_TYPE_NULL, FLEXI_UNPACK_TYPE(*v));
}

TEST(CursorInt, Blob_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const uint8_t *v = nullptr;
    flexi_ssize_t len = -1;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_blob(&cursor, &v, &len));
    ASSERT_EQ(0x00, *v);
    ASSERT_EQ(0, len);
}

TEST(CursorInt, Bool_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    bool v = false;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&cursor, &v));
    ASSERT_EQ(true, v);
}

TEST(CursorInt, SeekVectorIndex_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE,
        flexi_cursor_seek_vector_index(&cursor, 0, &v));
}

TEST(CursorInt, MapKeyAtIndex_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_map_key_at_index(&cursor, 0, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorInt, SeekMapKey_Int64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_seek_map_key(&cursor, "", &v));
}

TEST(CursorInt, Types_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    ASSERT_EQ(FLEXI_TYPE_UINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_width(&cursor));
    ASSERT_EQ(0, flexi_cursor_length(&cursor));
}

TEST(CursorInt, Sint_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    int64_t v = 1;
    ASSERT_EQ(FLEXI_ERR_RANGE, flexi_cursor_sint(&cursor, &v));
    ASSERT_EQ(INT64_MAX, v);
}

TEST(CursorInt, Uint_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    uint64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&cursor, &v));
    ASSERT_EQ(UINT64_PATTERN, v);
}

TEST(CursorInt, Float32_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    constexpr float ex = float(UINT64_PATTERN);
    float v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&cursor, &v));
    ASSERT_EQ(ex, v);
}

TEST(CursorInt, Float64_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    constexpr double ex = double(UINT64_PATTERN);
    double v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&cursor, &v));
    ASSERT_EQ(ex, v);
}

TEST(CursorInt, Key_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_key(&cursor, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorInt, String_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    const char *v = nullptr;
    flexi_ssize_t len = -1;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_string(&cursor, &v, &len));
    ASSERT_STREQ("", v);
    ASSERT_EQ(0, len);
}

TEST(CursorInt, TypedVectorData_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    const void *v = nullptr;
    flexi_type_e t = FLEXI_TYPE_NULL;
    int s = -1;
    flexi_ssize_t c = -1;

    ASSERT_EQ(FLEXI_ERR_BADTYPE,
        flexi_cursor_typed_vector_data(&cursor, &v, &t, &s, &c));
    ASSERT_EQ(0, *static_cast<const uint64_t *>(v));
    ASSERT_EQ(FLEXI_TYPE_INVALID, t);
    ASSERT_EQ(0, s);
    ASSERT_EQ(0, c);
}

TEST(CursorInt, VectorTypes_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    const flexi_packed_t *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_vector_types(&cursor, &v));
    ASSERT_EQ(FLEXI_TYPE_NULL, FLEXI_UNPACK_TYPE(*v));
}

TEST(CursorInt, Blob_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    const uint8_t *v = nullptr;
    flexi_ssize_t len = -1;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_blob(&cursor, &v, &len));
    ASSERT_EQ(0x00, *v);
    ASSERT_EQ(0, len);
}

TEST(CursorInt, Bool_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    bool v = false;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&cursor, &v));
    ASSERT_EQ(true, v);
}

TEST(CursorInt, SeekVectorIndex_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE,
        flexi_cursor_seek_vector_index(&cursor, 0, &v));
}

TEST(CursorInt, MapKeyAtIndex_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_map_key_at_index(&cursor, 0, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorInt, SeekMapKey_Uint64Pattern)
{
    flexi_cursor_s cursor{};
    GetCursorUint64Pattern(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_seek_map_key(&cursor, "", &v));
}
