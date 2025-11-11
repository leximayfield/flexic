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

TEST_F(WriteFixture, WriteMapInts)
{
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "bool"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "sint"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "indirect_sint"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "uint"));
    ASSERT_EQ(FLEXI_OK, flexi_write_key(&m_writer, "indirect_uint"));
    flexi_stack_idx_t keyset = -1;
    ASSERT_EQ(FLEXI_OK,
        flexi_write_map_keys(&m_writer, 5, FLEXI_WIDTH_2B, &keyset));
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
        'b', 'o', 'o', 'l', '\0', // First three keys
        's', 'i', 'n', 't', '\0', //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 's', 'i', 'n', 't',
        '\0',                     //
        'u', 'i', 'n', 't', '\0', // Last two keys
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 'u', 'i', 'n', 't',
        '\0',                         //
        0x05, 0x00,                   // Map keys vector length
        0x2d, 0x00,                   // Keys[0] "bool"
        0x25, 0x00,                   // Keys[1] "indirect_sint"
        0x14, 0x00,                   // Keys[2] "indirect_uint"
        0x2e, 0x00,                   // Keys[3] "sint"
        0x1d, 0x00,                   // Keys[4] "uint"
        0x00,                         // Padding
        0xff, 0xff, 0xff, 0x7f,       // Indirect int
        0xff, 0xff, 0xff, 0xff,       // Indirect uint
        0x13, 0x00,                   // Keys vector offset
        0x02, 0x00,                   // Keys vector stride
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
