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

#include "flexic.h"
#include "tests.hpp"

#include <nlohmann/json.hpp>

/******************************************************************************/

// [FIXME]: Dedupe 
static constexpr std::array<uint8_t, 55> g_basic_types = {0x4B, 0x65, 0x79,
    0x00, 0x03, 0x53, 0x74, 0x72, 0x00, 0x03, 0x04, 0x00, 0xDB, 0x0F, 0xC9,
    0x3F, 0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0x40, 0x04, 0x62, 0x6C,
    0x6F, 0x62, 0x0B, 0x00, 0x01, 0x02, 0x21, 0x1D, 0x1A, 0x1A, 0x19, 0x16,
    0x0E, 0x01, 0x00, 0x04, 0x08, 0x10, 0x14, 0x18, 0x1C, 0x22, 0x23, 0x64,
    0x68, 0x16, 0x28, 0x01};

TEST(CursorToJSON, One)
{
    flexi_span_s span =
        flexi_make_span(&g_basic_types[0], std::size(g_basic_types));

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));

    std::string json_str;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_to_json(&cursor, json_str));

    nlohmann::json json; 
    ASSERT_NO_THROW(json = nlohmann::json::parse(json_str));
}
