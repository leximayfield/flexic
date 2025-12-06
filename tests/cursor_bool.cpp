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
GetCursorBoolPattern(flexi_cursor_s &cursor)
{
    constexpr std::array<uint8_t, 3> s_data = {0x01, 0x68, 0x01};

    auto buffer = flexi_make_buffer(s_data.data(), s_data.size());
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
}

TEST(CursorBool, Types)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    ASSERT_EQ(FLEXI_TYPE_BOOL, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_width(&cursor));
    ASSERT_EQ(0, flexi_cursor_length(&cursor));
}

TEST(CursorBool, Sint)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    int64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&cursor, &v));
    ASSERT_EQ(1, v);
}

TEST(CursorBool, Uint)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    uint64_t v = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&cursor, &v));
    ASSERT_EQ(1, v);
}

TEST(CursorBool, Float32)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    float v = 0.0f;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&cursor, &v));
    ASSERT_EQ(1.0f, v);
}

TEST(CursorBool, Float64)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    double v = 0.0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&cursor, &v));
    ASSERT_EQ(1.0, v);
}

TEST(CursorBool, Key)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_key(&cursor, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorBool, String)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    const char *v = nullptr;
    flexi_ssize_t len = -1;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_string(&cursor, &v, &len));
    ASSERT_STREQ("", v);
    ASSERT_EQ(0, len);
}

TEST(CursorBool, TypedVectorData)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

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

TEST(CursorBool, VectorTypes)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    const flexi_packed_t *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_vector_types(&cursor, &v));
    ASSERT_EQ(FLEXI_TYPE_NULL, FLEXI_UNPACK_TYPE(*v));
}

TEST(CursorBool, Blob)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    const uint8_t *v = nullptr;
    flexi_ssize_t len = -1;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_blob(&cursor, &v, &len));
    ASSERT_EQ(0x00, *v);
    ASSERT_EQ(0, len);
}

TEST(CursorBool, Bool)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    bool v = false;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&cursor, &v));
    ASSERT_EQ(true, v);
}

TEST(CursorBool, SeekVectorIndex)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE,
        flexi_cursor_seek_vector_index(&cursor, 0, &v));
}

TEST(CursorBool, MapKeyAtIndex)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    const char *v = nullptr;
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_map_key_at_index(&cursor, 0, &v));
    ASSERT_STREQ("", v);
}

TEST(CursorBool, SeekMapKey)
{
    flexi_cursor_s cursor{};
    GetCursorBoolPattern(cursor);

    flexi_cursor_s v{};
    ASSERT_EQ(FLEXI_ERR_BADTYPE, flexi_cursor_seek_map_key(&cursor, "", &v));
}
