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
GetCursorStringPattern(flexi_cursor_s &cursor)
{
    static constexpr std::array<uint8_t, 10> s_data = {
        0x05, 0x78, 0x79, 0x7A, 0x7A, 0x79, 0x00, // String
        0x06, 0x14, 0x01,                         // Root
    };

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

TEST_CASE("Cursor metadata", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    REQUIRE(FLEXI_TYPE_STRING == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));
}

TEST_CASE("flexi_cursor_sint", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    int64_t v = 1;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_sint(&cursor, &v));
    REQUIRE(0 == v);
}

TEST_CASE("flexi_cursor_uint", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    uint64_t v = 1;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_uint(&cursor, &v));
    REQUIRE(0 == v);
}

TEST_CASE("flexi_cursor_f32", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    float v = 1.0f;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_f32(&cursor, &v));
    REQUIRE(0.0 == v);
}

TEST_CASE("flexi_cursor_f64", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    double v = 1.0;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_f64(&cursor, &v));
    REQUIRE(0.0 == v);
}

TEST_CASE("flexi_cursor_key", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    const char *v = nullptr;
    REQUIRE(FLEXI_OK == flexi_cursor_key(&cursor, &v));
    REQUIRE(!strcmp("xyzzy", v));
}

TEST_CASE("flexi_cursor_string", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    const char *v = nullptr;
    flexi_ssize_t len = -1;
    REQUIRE(FLEXI_OK == flexi_cursor_string(&cursor, &v, &len));
    REQUIRE(!strcmp("xyzzy", v));
    REQUIRE(5 == len);
}

TEST_CASE("flexi_cursor_typed_vector_data", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    const void *v = nullptr;
    flexi_type_e t = FLEXI_TYPE_NULL;
    int s = -1;
    flexi_ssize_t c = -1;

    REQUIRE(FLEXI_ERR_BADTYPE ==
            flexi_cursor_typed_vector_data(&cursor, &v, &t, &s, &c));
    REQUIRE(0 == *static_cast<const int64_t *>(v));
    REQUIRE(FLEXI_TYPE_INVALID == t);
    REQUIRE(0 == s);
    REQUIRE(0 == c);
}

TEST_CASE("flexi_cursor_vector_types", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    const flexi_packed_t *v = nullptr;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_vector_types(&cursor, &v));
    REQUIRE(FLEXI_TYPE_NULL == FLEXI_UNPACK_TYPE(*v));
}

TEST_CASE("flexi_cursor_blob", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    const uint8_t *v = nullptr;
    flexi_ssize_t len = -1;
    REQUIRE(FLEXI_OK == flexi_cursor_blob(&cursor, &v, &len));
    REQUIRE(!memcmp("xyzzy", v, 5));
    REQUIRE(5 == len);
}

TEST_CASE("flexi_cursor_bool", "[cursor_string]")
{
    flexi_cursor_s cursor{};
    GetCursorStringPattern(cursor);

    bool v = true;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_bool(&cursor, &v));
    REQUIRE(false == v);
}

/******************************************************************************/

TEST_CASE("Malicious String - OOB length", "[cursor_string]")
{
    constexpr std::array<uint8_t, 10> s_data = {
        0xFF, 0x78, 0x79, 0x7A, 0x7A, 0x79, 0x00, // String
        0x06, 0x14, 0x01,                         // Root
    };

    flexi_cursor_s cursor{};
    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_ERR_BADREAD == flexi_open_span(&span, &cursor));
};

/******************************************************************************/

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
