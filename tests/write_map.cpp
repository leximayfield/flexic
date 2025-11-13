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

TEST_F(WriteFixture, MapInts)
{
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "bool"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "sint"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "indirect_sint"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "uint"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "indirect_uint"));
    flexi_stack_idx_t keyset = -1;
    ASSERT_EQ(FLEXI_OK,
        flexi_write_map_keys(&m_writer, 5, FLEXI_WIDTH_1B, &keyset));
    ASSERT_EQ(0, keyset);

    ASSERT_EQ(FLEXI_OK, flexi_write_bool_keyed(&m_writer, "bool", true));
    ASSERT_EQ(FLEXI_OK, flexi_write_sint_keyed(&m_writer, "sint", INT16_MAX));
    ASSERT_EQ(FLEXI_OK,
        flexi_write_indirect_sint_keyed(&m_writer, "indirect_sint", INT32_MAX));
    ASSERT_EQ(FLEXI_OK, flexi_write_uint_keyed(&m_writer, "uint", UINT16_MAX));
    ASSERT_EQ(FLEXI_OK, flexi_write_indirect_uint_keyed(&m_writer,
                            "indirect_uint", UINT32_MAX));
    ASSERT_EQ(FLEXI_OK, flexi_write_map(&m_writer, keyset, 5, FLEXI_WIDTH_2B));
    ASSERT_EQ(FLEXI_OK, flexi_write_finalize(&m_writer));

    std::vector<uint8_t> expected = {
        'b', 'o', 'o', 'l', '\0', // Key values
        's', 'i', 'n', 't', '\0', //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 's', 'i', 'n', 't',
        '\0',                     //
        'u', 'i', 'n', 't', '\0', //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 'u', 'i', 'n', 't',
        '\0',                         //
        0x05,                         // Map keys vector length
        0x2c,                         // Keys[0] "bool"
        0x23,                         // Keys[1] "indirect_sint"
        0x11,                         // Keys[2] "indirect_uint"
        0x2a,                         // Keys[3] "sint"
        0x18,                         // Keys[4] "uint"
        0x00, 0x00, 0x00,             // Padding
        0xff, 0xff, 0xff, 0x7f,       // Indirect int
        0xff, 0xff, 0xff, 0xff,       // Indirect uint
        0x10, 0x00,                   // Keys vector offset
        0x01, 0x00,                   // Keys vector stride
        0x05, 0x00,                   // Map values vector length
        0x01, 0x00,                   // Values[0] Bool
        0x10, 0x00,                   // Values[1] Indirect int
        0x0e, 0x00,                   // Values[2] Indirect uint
        0xff, 0x7f,                   // Values[3] Int
        0xff, 0xff,                   // Values[4] Uint
        0x68, 0x1a, 0x1e, 0x05, 0x09, // Types
        0x0f, 0x25, 0x01,             // Root
    };

    AssertData(expected);

    flexi_cursor_s cursor{};
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_TYPE_MAP, flexi_cursor_type(&cursor));
    ASSERT_EQ(2, flexi_cursor_width(&cursor));

    flexi_cursor_s value{};

    bool vbool = false;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_seek_map_key(&cursor, "bool", &value));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&value, &vbool));
    ASSERT_EQ(true, vbool);

    int64_t vsint = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_seek_map_key(&cursor, "sint", &value));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&value, &vsint));
    ASSERT_EQ(INT16_MAX, vsint);
    ASSERT_EQ(FLEXI_OK,
        flexi_cursor_seek_map_key(&cursor, "indirect_sint", &value));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&value, &vsint));
    ASSERT_EQ(INT32_MAX, vsint);

    uint64_t vuint = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_seek_map_key(&cursor, "uint", &value));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&value, &vuint));
    ASSERT_EQ(UINT16_MAX, vuint);
    ASSERT_EQ(FLEXI_OK,
        flexi_cursor_seek_map_key(&cursor, "indirect_uint", &value));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&value, &vuint));
    ASSERT_EQ(UINT32_MAX, vuint);
}

TEST_F(WriteFixture, MapFloats)
{
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "f32"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "indirect_f32"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "f64"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "indirect_f64"));
    flexi_stack_idx_t keyset = -1;
    ASSERT_EQ(FLEXI_OK,
        flexi_write_map_keys(&m_writer, 4, FLEXI_WIDTH_1B, &keyset));
    ASSERT_EQ(0, keyset);

    ASSERT_EQ(FLEXI_OK, flexi_write_f32_keyed(&m_writer, "f32", PI_VALUE));
    ASSERT_EQ(FLEXI_OK,
        flexi_write_indirect_f32_keyed(&m_writer, "indirect_f32", PI_VALUE));
    ASSERT_EQ(FLEXI_OK, flexi_write_f64_keyed(&m_writer, "f64", PI_VALUE));
    ASSERT_EQ(FLEXI_OK,
        flexi_write_indirect_f64_keyed(&m_writer, "indirect_f64", PI_VALUE));
    ASSERT_EQ(FLEXI_OK, flexi_write_map(&m_writer, keyset, 4, FLEXI_WIDTH_8B));
    ASSERT_EQ(FLEXI_OK, flexi_write_finalize(&m_writer));

    std::vector<uint8_t> expected = {'f', '3', '2', '\0',                 //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 'f', '3', '2', '\0', //
        'f', '6', '4', '\0',                                              //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 'f', '6', '4', '\0', //
        // Map keys vector length
        0x04,
        // Keys[0] "f32"
        0x23,
        // Keys[1] "f64"
        0x13,
        // Keys[2] "indirect_f32"
        0x21,
        // Keys[3] "indirect_f64"
        0x11,
        // Padding
        0x00,
        // Indirect float
        0xdb, 0x0f, 0x49, 0x40,
        // Padding
        0x00, 0x00, 0x00, 0x00,
        // Indirect double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // Keys vector offset (stride 8)
        0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Keys vector stride (stride 8)
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Vector length (stride 8)
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [0] Float (widened)
        0x00, 0x00, 0x00, 0x60, 0xfb, 0x21, 0x09, 0x40,
        // [1] Double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // [2] Indirect float
        0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [3] Indirect double
        0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Vector types
        0x0e, 0x0f, 0x22, 0x23,
        // Root
        0x24, 0x27, 0x01};

    AssertData(expected);

    flexi_cursor_s cursor{};
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_TYPE_MAP, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_width(&cursor));
    ASSERT_EQ(4, flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};
    float f32 = 0.0f;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_seek_map_key(&cursor, "f32", &vcursor));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&vcursor, &f32));
    ASSERT_FLOAT_EQ(f32, PI_VALUE);

    f32 = 0.0f;
    ASSERT_EQ(FLEXI_OK,
        flexi_cursor_seek_map_key(&cursor, "indirect_f32", &vcursor));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&vcursor, &f32));
    ASSERT_FLOAT_EQ(f32, PI_VALUE);

    double f64 = 0.0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_seek_map_key(&cursor, "f64", &vcursor));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&vcursor, &f64));
    ASSERT_DOUBLE_EQ(f64, PI_VALUE);

    f64 = 0.0;
    ASSERT_EQ(FLEXI_OK,
        flexi_cursor_seek_map_key(&cursor, "indirect_f64", &vcursor));
    ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&vcursor, &f64));
    ASSERT_DOUBLE_EQ(f64, PI_VALUE);
}
