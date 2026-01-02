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
GetCursorInt64Pattern(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 10> s_data = {
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x07, 0x08};

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

/******************************************************************************/

TEST_CASE("Cursor metadata", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    REQUIRE(FLEXI_TYPE_SINT == flexi_cursor_type(&cursor));
    REQUIRE(8 == flexi_cursor_width(&cursor));
    REQUIRE(0 == flexi_cursor_length(&cursor));
}

TEST_CASE("flexi_cursor_sint", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    int64_t v = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_sint(&cursor, &v));
    REQUIRE(INT64_PATTERN == v);
}

TEST_CASE("flexi_cursor_uint", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    uint64_t v = 1;
    REQUIRE(FLEXI_ERR_RANGE == flexi_cursor_uint(&cursor, &v));
    REQUIRE(0 == v);
}

TEST_CASE("flexi_cursor_f32", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    constexpr float ex = float(INT64_PATTERN);
    float v = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_f32(&cursor, &v));
    REQUIRE_THAT(ex, WithinRel(v));
}

TEST_CASE("flexi_cursor_f64", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    constexpr double ex = double(INT64_PATTERN);
    double v = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_f64(&cursor, &v));
    REQUIRE_THAT(ex, WithinRel(v));
}

TEST_CASE("flexi_cursor_key", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const char *v = nullptr;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_key(&cursor, &v));
    REQUIRE_THAT("", Equals(v));
}

TEST_CASE("flexi_cursor_string", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const char *v = nullptr;
    flexi_ssize_t len = -1;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_string(&cursor, &v, &len));
    REQUIRE_THAT("", Equals(v));
    REQUIRE(0 == len);
}

TEST_CASE("flexi_cursor_typed_vector_data", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

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

TEST_CASE("flexi_cursor_vector_types", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const flexi_packed_t *v = nullptr;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_vector_types(&cursor, &v));
    REQUIRE(FLEXI_TYPE_NULL == FLEXI_UNPACK_TYPE(*v));
}

TEST_CASE("flexi_cursor_blob", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const uint8_t *v = nullptr;
    flexi_ssize_t len = -1;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_blob(&cursor, &v, &len));
    REQUIRE(0x00 == *v);
    REQUIRE(0 == len);
}

TEST_CASE("flexi_cursor_bool", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    bool v = false;
    REQUIRE(FLEXI_OK == flexi_cursor_bool(&cursor, &v));
    REQUIRE(true == v);
}

TEST_CASE("flexi_cursor_seek_vector_index", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    flexi_cursor_s v{};
    REQUIRE(FLEXI_ERR_BADTYPE ==
            flexi_cursor_seek_vector_index(&cursor, 0, &v));
}

TEST_CASE("flexi_cursor_map_key_at_index", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    const char *v = nullptr;
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_map_key_at_index(&cursor, 0, &v));
    REQUIRE_THAT("", Equals(v));
}

TEST_CASE("flexi_cursor_seek_map_key", "[cursor_sint]")
{
    flexi_cursor_s cursor{};
    GetCursorInt64Pattern(cursor);

    flexi_cursor_s v{};
    REQUIRE(FLEXI_ERR_BADTYPE == flexi_cursor_seek_map_key(&cursor, "", &v));
}
