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

static bool
ForeachMap(const char *key, flexi_cursor_s *cursor, void *user)
{
    return true;
}

TEST_F(WriteFixture, ForeachMap)
{
    ASSERT_EQ(FLEXI_OK, flexi_write_uint(&m_writer, "uint", UINT16_MAX));
    ASSERT_EQ(FLEXI_OK, flexi_write_sint(&m_writer, "sint", INT16_MAX));
    ASSERT_EQ(FLEXI_OK,
        flexi_write_indirect_uint(&m_writer, "indirect_uint", UINT32_MAX));
    ASSERT_EQ(FLEXI_OK,
        flexi_write_indirect_sint(&m_writer, "indirect_sint", INT32_MAX));
    ASSERT_EQ(FLEXI_OK, flexi_write_bool(&m_writer, "bool", true));
    ASSERT_EQ(FLEXI_OK, flexi_write_map(&m_writer, NULL, 5, FLEXI_WIDTH_2B));
    ASSERT_EQ(FLEXI_OK, flexi_write_finalize(&m_writer));

    flexi_cursor_s cursor;
    GetCursor(&cursor);

    ASSERT_EQ(FLEXI_OK, flexi_cursor_foreach(&cursor, &ForeachMap, NULL));
}
