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

/******************************************************************************/

static void
GetCursorLargeDoc1(flexi_cursor_s &cursor)
{
    static std::string s_data = ReadFileToString("large_doc1.flexbuf");

    auto buffer = flexi_make_buffer(s_data.data(), s_data.size());
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
}

/******************************************************************************/

TEST(CursorMap, SeekMapKey)
{
    flexi_cursor_s cursor;
    GetCursorLargeDoc1(cursor);

    flexi_cursor_s curValue;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_seek_map_key(&cursor, "map-23", &curValue));
}
