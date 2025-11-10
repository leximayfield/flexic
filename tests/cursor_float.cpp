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
GetCursorPiValue32(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 6> s_data = {0xdb, 0x0f, 0x49, 0x40, 0x0e, 0x04};

    auto buffer = flexi_make_buffer(s_data.data(), s_data.size());
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
}

static void
GetCursorInf32(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 6> s_data = {0x00, 0x00, 0x80, 0x7f, 0x0e, 0x04};

    auto buffer = flexi_make_buffer(s_data.data(), s_data.size());
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
}

static void
GetCursorPiValue64(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 10> s_data = {
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, 0x0f, 0x08};

    auto buffer = flexi_make_buffer(s_data.data(), s_data.size());
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
}

TEST(CursorFloat, Types_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_width(&cursor));
    ASSERT_EQ(0, flexi_cursor_length(&cursor));
}

TEST(CursorFloat, Sint_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    int64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&cursor, &v));
    ASSERT_EQ(3, v);
}

TEST(CursorFloat, Sint_Inf32)
{
    flexi_cursor_s cursor{};
    GetCursorInf32(cursor);

    int64_t v = 0;
    ASSERT_EQ(FLEXI_ERR_RANGE, flexi_cursor_sint(&cursor, &v));
    ASSERT_EQ(INT64_MAX, v);
}

TEST(CursorFloat, Uint_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    uint64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&cursor, &v));
    ASSERT_EQ(3, v);
}

TEST(CursorFloat, Float32_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    float v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&cursor, &v));
    ASSERT_FLOAT_EQ(PI_VALUE, v);
}

TEST(CursorFloat, Float64_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    double v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&cursor, &v));
    ASSERT_FLOAT_EQ(PI_VALUE, v);
}

TEST(CursorFloat, Key_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_key(&cursor, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorFloat, String_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_string(&cursor, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorFloat, TypedVectorData_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    const void *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_typed_vector_data(&cursor, &v));
    ASSERT_STREQ("", static_cast<const char *>(v));
}

TEST(CursorFloat, VectorTypes_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    const flexi_packed_t *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_vector_types(&cursor, &v));
    ASSERT_EQ(nullptr, v);
}

TEST(CursorFloat, Blob_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    const uint8_t *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_blob(&cursor, &v));
    ASSERT_STREQ("", reinterpret_cast<const char *>(v));
}

TEST(CursorFloat, Bool_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    bool v = false;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&cursor, &v));
    ASSERT_EQ(true, v);
}

TEST(CursorFloat, SeekVectorIndex_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE,
        flexi_cursor_seek_vector_index(&cursor, 0, &v));
}

TEST(CursorFloat, MapKeyAtIndex_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_map_key_at_index(&cursor, 0, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorFloat, SeekMapKey_PiValue32)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue32(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_seek_map_key(&cursor, "", &v));
}

TEST(CursorFloat, Types_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_width(&cursor));
    ASSERT_EQ(0, flexi_cursor_length(&cursor));
}

TEST(CursorFloat, Sint_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    int64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&cursor, &v));
    ASSERT_EQ(3, v);
}

TEST(CursorFloat, Uint_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    uint64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&cursor, &v));
    ASSERT_EQ(3, v);
}

TEST(CursorFloat, Float32_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    float v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&cursor, &v));
    ASSERT_FLOAT_EQ(PI_VALUE, v);
}

TEST(CursorFloat, Float64_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    double v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&cursor, &v));
    ASSERT_EQ(PI_VALUE, v);
}

TEST(CursorFloat, Key_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_key(&cursor, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorFloat, String_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_string(&cursor, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorFloat, TypedVectorData_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    const void *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_typed_vector_data(&cursor, &v));
    ASSERT_STREQ("", static_cast<const char *>(v));
}

TEST(CursorFloat, VectorTypes_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    const flexi_packed_t *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_vector_types(&cursor, &v));
    ASSERT_EQ(nullptr, v);
}

TEST(CursorFloat, Blob_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    const uint8_t *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_blob(&cursor, &v));
    ASSERT_STREQ("", reinterpret_cast<const char *>(v));
}

TEST(CursorFloat, Bool_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    bool v = false;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&cursor, &v));
    ASSERT_EQ(true, v);
}

TEST(CursorFloat, SeekVectorIndex_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE,
        flexi_cursor_seek_vector_index(&cursor, 0, &v));
}

TEST(CursorFloat, MapKeyAtIndex_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_map_key_at_index(&cursor, 0, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorFloat, SeekMapKey_PiValue64)
{
    flexi_cursor_s cursor{};
    GetCursorPiValue64(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_seek_map_key(&cursor, "", &v));
}
