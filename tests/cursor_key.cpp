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

constexpr std::array<uint8_t, 9> g_test_key = {
    0x78, 0x79, 0x7A, 0x7A, 0x79, 0x00, // Key
    0x06, 0x10, 0x01,                   // Root
};

TEST_CASE("Valid key length", "[cursor_key]")
{
    flexi_cursor_s cursor{};
    auto span = flexi_make_span(g_test_key.data(), g_test_key.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));
}

/******************************************************************************/

constexpr std::array<uint8_t, 9> g_bad_key = {
    0x78, 0x79, 0x7A, 0x7A, 0x79, 0x01, // Malicious key
    0x06, 0x10, 0x01,                   // Root
};

TEST_CASE("Malicious key length", "[cursor_key]")
{
    flexi_cursor_s cursor{};
    auto span = flexi_make_span(g_bad_key.data(), g_bad_key.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
    REQUIRE(-1 == flexi_cursor_length(&cursor));
}
