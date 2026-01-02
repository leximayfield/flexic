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
GetCursorLargeStrings(flexi_cursor_s &cursor)
{
    static std::string s_data = ReadFileToString("large_strings.flexbuf");

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

TEST_CASE("Omnibus Test", "[cursor_string]")
{
    flexi_cursor_s cursor{}, str_cursor{};
    const char *str = nullptr;
    flexi_ssize_t len = -1;
    GetCursorLargeStrings(cursor);

    REQUIRE(FLEXI_OK ==
            flexi_cursor_seek_vector_index(&cursor, 0, &str_cursor));
    REQUIRE(255 == flexi_cursor_length(&str_cursor));
    REQUIRE(FLEXI_OK == flexi_cursor_string(&str_cursor, &str, &len));
    REQUIRE('x' == str[254]);
    REQUIRE('\0' == str[255]);
    REQUIRE(255 == len);

    REQUIRE(FLEXI_OK ==
            flexi_cursor_seek_vector_index(&cursor, 1, &str_cursor));
    REQUIRE(384 == flexi_cursor_length(&str_cursor));
    REQUIRE(FLEXI_OK == flexi_cursor_string(&str_cursor, &str, &len));
    REQUIRE('y' == str[383]);
    REQUIRE('\0' == str[384]);
    REQUIRE(384 == len);

    REQUIRE(FLEXI_OK ==
            flexi_cursor_seek_vector_index(&cursor, 2, &str_cursor));
    REQUIRE(65540 == flexi_cursor_length(&str_cursor));
    REQUIRE(FLEXI_OK == flexi_cursor_string(&str_cursor, &str, &len));
    REQUIRE('z' == str[65539]);
    REQUIRE('\0' == str[65540]);
    REQUIRE(65540 == len);
}
