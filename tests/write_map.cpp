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

TEST(Writer, WriteInlineMapInts) {
    TestStream stream;

    {
        flexi_writer_s writer{};

        writer.user = &stream;
        writer.write_func = TestStream::WriteFunc;
        writer.data_at_func = TestStream::DataAtFunc;
        writer.tell_func = TestStream::TellFunc;

        ASSERT_TRUE(flexi_write_key(&writer, "bool"));
        ASSERT_TRUE(flexi_write_bool(&writer, true));
        ASSERT_TRUE(flexi_write_key(&writer, "sint"));
        ASSERT_TRUE(flexi_write_sint(&writer, INT16_MAX));
        ASSERT_TRUE(flexi_write_key(&writer, "indirect_sint"));
        ASSERT_TRUE(flexi_write_indirect_sint(&writer, INT32_MAX));
        ASSERT_TRUE(flexi_write_key(&writer, "uint"));
        ASSERT_TRUE(flexi_write_uint(&writer, UINT16_MAX));
        ASSERT_TRUE(flexi_write_key(&writer, "indirect_uint"));
        ASSERT_TRUE(flexi_write_indirect_uint(&writer, UINT32_MAX));
        ASSERT_TRUE(flexi_write_inline_map(&writer, 5, FLEXI_WIDTH_2B));
        ASSERT_TRUE(flexi_write_finalize(&writer));
    }

    std::array<uint8_t, 87> expected = {
        'b',  'o',  'o',  'l',  '\0', // First three keys
        's',  'i',  'n',  't',  '\0', //
        'i',  'n',  'd',  'i',  'r',  'e', 'c',
        't',  '_',  's',  'i',  'n',  't', '\0', //
        0xff, 0xff, 0xff, 0x7f,                  // Indirect int
        'u',  'i',  'n',  't',  '\0',            // Last two keys
        'i',  'n',  'd',  'i',  'r',  'e', 'c',
        't',  '_',  'u',  'i',  'n',  't', '\0', //
        0xff, 0xff, 0xff, 0xff,                  // Indirect uint
        0x05, 0x00,                              // Map keys vector length
        0x35, 0x00,                              // Keys[0] "bool"
        0x2d, 0x00,                              // Keys[1] "indirect_sint"
        0x18, 0x00,                              // Keys[2] "indirect_uint"
        0x36, 0x00,                              // Keys[3] "sint"
        0x21, 0x00,                              // Keys[4] "uint"
        0x0a, 0x00,                              // Keys vector offset
        0x02, 0x00,                              // Keys vector stride
        0x05, 0x00,                              // Map values vector length
        0x01, 0x00,                              // Values[0] Bool
        0x2f, 0x00,                              // Values[1] Indirect int
        0x1a, 0x00,                              // Values[2] Indirect uint
        0xff, 0x7f,                              // Values[3] Int
        0xff, 0xff,                              // Values[4] Uint
        0x1e, 0x13, 0x05, 0x13, 0x09,            // Types
        0x0f, 0x25, 0x01,                        // Root
    };

    for (size_t i = 0; i < std::size(expected); i++) {
        ASSERT_EQ(expected[i], *stream.DataAt(i));
    }
}
