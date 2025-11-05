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

TEST_F(WriteFixture, Null) {
    ASSERT_TRUE(flexi_write_null(&m_writer));
    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    std::vector<uint8_t> expected = {0x00, 0x00, 0x01};
    AssertData(expected);

    flexi_cursor_s cursor{};
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_TYPE_NULL, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_width(&cursor));
}

TEST_F(WriteFixture, Bool) {
    ASSERT_TRUE(flexi_write_bool(&m_writer, true));
    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    std::vector<uint8_t> expected = {0x01, 0x68, 0x01};
    AssertData(expected);

    flexi_cursor_s cursor{};
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_TYPE_BOOL, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_width(&cursor));

    bool value = false;
    ASSERT_TRUE(flexi_cursor_bool(&cursor, &value));
    ASSERT_EQ(true, value);
}
