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

TEST_F(WriteFixture, VectorInts)
{
    ASSERT_TRUE(flexi_write_bool(&m_writer, true));
    ASSERT_TRUE(flexi_write_sint(&m_writer, INT16_MAX));
    ASSERT_TRUE(flexi_write_indirect_sint(&m_writer, INT32_MAX));
    ASSERT_TRUE(flexi_write_uint(&m_writer, UINT16_MAX));
    ASSERT_TRUE(flexi_write_indirect_uint(&m_writer, UINT32_MAX));
    ASSERT_TRUE(flexi_write_vector(&m_writer, 5, FLEXI_WIDTH_2B));
    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    std::vector<uint8_t> expected = {
        0xff, 0xff, 0xff, 0x7f,       // Indirect int
        0xff, 0xff, 0xff, 0xff,       // Indirect uint
        0x05, 0x00,                   // Vector length (stride 2)
        0x01, 0x00,                   // [0] Bool
        0xff, 0x7f,                   // [1] Int
        0x0e, 0x00,                   // [2] Indirect int
        0xff, 0xff,                   // [3] Uint
        0x0e, 0x00,                   // [4] Indirect uint
        0x68, 0x05, 0x1a, 0x09, 0x1e, // Vector types
        0x0f, 0x29, 0x01,             // Root offset
    };

    AssertData(expected);

    flexi_cursor_s cursor{};
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR, flexi_cursor_type(&cursor));
    ASSERT_EQ(2, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};
    bool b = false;

    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    ASSERT_TRUE(flexi_cursor_bool(&vcursor, &b));
    ASSERT_EQ(b, true);

    int64_t s64 = 0;

    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    ASSERT_TRUE(flexi_cursor_sint(&vcursor, &s64));
    ASSERT_EQ(s64, INT16_MAX);
    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 2, &vcursor));
    ASSERT_TRUE(flexi_cursor_sint(&vcursor, &s64));
    ASSERT_EQ(s64, INT32_MAX);

    uint64_t u64 = 0;

    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 3, &vcursor));
    ASSERT_TRUE(flexi_cursor_uint(&vcursor, &u64));
    ASSERT_EQ(u64, UINT16_MAX);
    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 4, &vcursor));
    ASSERT_TRUE(flexi_cursor_uint(&vcursor, &u64));
    ASSERT_EQ(u64, UINT32_MAX);
}

TEST_F(WriteFixture, VectorFloats)
{
    ASSERT_TRUE(flexi_write_f32(&m_writer, PI_VALUE));
    ASSERT_TRUE(flexi_write_indirect_f32(&m_writer, PI_VALUE));
    ASSERT_TRUE(flexi_write_f64(&m_writer, PI_VALUE));
    ASSERT_TRUE(flexi_write_indirect_f64(&m_writer, PI_VALUE));
    ASSERT_TRUE(flexi_write_vector(&m_writer, 4, FLEXI_WIDTH_8B));
    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    std::vector<uint8_t> expected = {//
        // Indirect float
        0xdb, 0x0f, 0x49, 0x40,
        // Indirect double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // Vector length (stride 8)
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [0] Float (widened)
        0x00, 0x00, 0x00, 0x60, 0xfb, 0x21, 0x09, 0x40,
        // [1] Indirect float
        0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [2] Double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // [2] Indirect double
        0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Vector types
        0x0e, 0x22, 0x0f, 0x23,
        // Root
        0x24, 0x2b, 0x01};

    AssertData(expected);

    flexi_cursor_s cursor{};
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_width(&cursor));
    ASSERT_EQ(4, flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};
    float f32 = 0.0f;

    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    ASSERT_TRUE(flexi_cursor_f32(&vcursor, &f32));
    ASSERT_FLOAT_EQ(f32, PI_VALUE);
    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    ASSERT_TRUE(flexi_cursor_f32(&vcursor, &f32));
    ASSERT_FLOAT_EQ(f32, PI_VALUE);

    double f64 = 0.0;

    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 2, &vcursor));
    ASSERT_TRUE(flexi_cursor_f64(&vcursor, &f64));
    ASSERT_DOUBLE_EQ(f64, PI_VALUE);
    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 3, &vcursor));
    ASSERT_TRUE(flexi_cursor_f64(&vcursor, &f64));
    ASSERT_DOUBLE_EQ(f64, PI_VALUE);
}

TEST_F(WriteFixture, StringBlob)
{
    constexpr std::array<uint8_t, 8> BLOB = {
        0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};

    ASSERT_TRUE(flexi_write_string(&m_writer, "xyzzy"));
    ASSERT_TRUE(flexi_write_blob(&m_writer, &BLOB[0], std::size(BLOB)));
    ASSERT_TRUE(flexi_write_vector(&m_writer, 2, FLEXI_WIDTH_1B));
    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    std::vector<uint8_t> expected = {// String
        0x05, 'x', 'y', 'z', 'z', 'y', '\0',
        // Blob
        0x08, 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1,
        // Vector length (stride 1)
        0x02,
        // [0] String offset
        0x10,
        // [1] Blob offset
        0x0a,
        // Vector types
        0x14, 0x64,
        // Root
        0x04, 0x28, 0x01};

    AssertData(expected);

    flexi_cursor_s cursor{};
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_width(&cursor));
    ASSERT_EQ(2, flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};

    const char *str = nullptr;
    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    ASSERT_TRUE(flexi_cursor_string(&vcursor, &str));
    ASSERT_EQ(5, flexi_cursor_length(&vcursor));
    ASSERT_STREQ(str, "xyzzy");

    const uint8_t *blob = nullptr;
    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    ASSERT_TRUE(flexi_cursor_blob(&vcursor, &blob));
    ASSERT_EQ(8, flexi_cursor_length(&vcursor));
    for (size_t i = 0; i < std::size(BLOB); i++) {
        ASSERT_EQ(blob[i], BLOB[i]);
    }
}
