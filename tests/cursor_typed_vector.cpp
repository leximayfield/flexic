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

/******************************************************************************/

static void
GetCursorFiveSint(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 27> s_data = {
        0x05, 0x00, 0x00, 0x00, // Vector length
        0x01, 0x00, 0x00, 0x00, // Vector[0] (1)
        0x02, 0x00, 0x00, 0x00, // Vector[1] (2)
        0x03, 0x00, 0x00, 0x00, // Vector[2] (3)
        0x04, 0x00, 0x00, 0x00, // Vector[3] (4)
        0x05, 0x00, 0x00, 0x00, // Vector[4] (5)
        0x14, 0x2e, 0x01        // Root
    };

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

TEST_CASE("flexi_cursor_typed_vector_data (Sint)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveSint(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_SINT == flexi_cursor_type(&cursor));
    REQUIRE(4 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    REQUIRE(FLEXI_TYPE_VECTOR_SINT == act_type);
    REQUIRE(4 == act_stride);
    REQUIRE(5 == act_count);

    const int32_t *vec = static_cast<const int32_t *>(act_data);
    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 4);
    REQUIRE(vec[4] == 5);
}

TEST_CASE("flexi_cursor_seek_vector_index (Sint)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveSint(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_SINT == flexi_cursor_type(&cursor));
    REQUIRE(4 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        REQUIRE(FLEXI_OK ==
                flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        REQUIRE(FLEXI_TYPE_SINT == flexi_cursor_type(&vcursor));
        REQUIRE(4 == flexi_cursor_width(&cursor));

        int64_t v = 0;
        REQUIRE(FLEXI_OK == flexi_cursor_sint(&vcursor, &v));
        REQUIRE(1 + int64_t(i) == v);
    }
}

/******************************************************************************/

static void
GetCursorFiveUint(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 27> s_data = {
        0x05, 0x00, 0x00, 0x00, // Vector length
        0x01, 0x00, 0x00, 0x00, // Vector[0] (1)
        0x02, 0x00, 0x00, 0x00, // Vector[1] (2)
        0x03, 0x00, 0x00, 0x00, // Vector[2] (3)
        0x04, 0x00, 0x00, 0x00, // Vector[3] (4)
        0x05, 0x00, 0x00, 0x00, // Vector[4] (5)
        0x14, 0x32, 0x01        // Root
    };

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

TEST_CASE("flexi_cursor_typed_vector_data (Uint)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveUint(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_UINT == flexi_cursor_type(&cursor));
    REQUIRE(4 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    REQUIRE(FLEXI_TYPE_VECTOR_UINT == act_type);
    REQUIRE(4 == act_stride);
    REQUIRE(5 == act_count);

    const uint32_t *vec = static_cast<const uint32_t *>(act_data);
    REQUIRE(vec[0] == 1);
    REQUIRE(vec[1] == 2);
    REQUIRE(vec[2] == 3);
    REQUIRE(vec[3] == 4);
    REQUIRE(vec[4] == 5);
}

TEST_CASE("flexi_cursor_seek_vector_index (Uint)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveUint(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_UINT == flexi_cursor_type(&cursor));
    REQUIRE(4 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        REQUIRE(FLEXI_OK ==
                flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        REQUIRE(FLEXI_TYPE_UINT == flexi_cursor_type(&vcursor));
        REQUIRE(4 == flexi_cursor_width(&cursor));

        uint64_t v = 0;
        REQUIRE(FLEXI_OK == flexi_cursor_uint(&vcursor, &v));
        REQUIRE(1 + uint64_t(i) == v);
    }
}

/******************************************************************************/

static void
GetCursorFiveFloat32(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 27> s_data = {
        0x05, 0x00, 0x00, 0x00, // Vector length
        0x00, 0x00, 0x80, 0x3f, // Vector[0] (1.0f)
        0x00, 0x00, 0x00, 0x40, // Vector[1] (2.0f)
        0x00, 0x00, 0x40, 0x40, // Vector[2] (3.0f)
        0x00, 0x00, 0x80, 0x40, // Vector[3] (4.0f)
        0x00, 0x00, 0xa0, 0x40, // Vector[4] (5.0f)
        0x14, 0x36, 0x01        // Root
    };

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

TEST_CASE("flexi_cursor_typed_vector_data (Float32)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveFloat32(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_FLOAT == flexi_cursor_type(&cursor));
    REQUIRE(4 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    REQUIRE(FLEXI_TYPE_VECTOR_FLOAT == act_type);
    REQUIRE(4 == act_stride);
    REQUIRE(5 == act_count);

    const float *vec = static_cast<const float *>(act_data);
    REQUIRE_THAT(vec[0], WithinRel(1.0f));
    REQUIRE_THAT(vec[1], WithinRel(2.0f));
    REQUIRE_THAT(vec[2], WithinRel(3.0f));
    REQUIRE_THAT(vec[3], WithinRel(4.0f));
    REQUIRE_THAT(vec[4], WithinRel(5.0f));
}

TEST_CASE("flexi_cursor_seek_vector_index (Float32)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveFloat32(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_FLOAT == flexi_cursor_type(&cursor));
    REQUIRE(4 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        REQUIRE(FLEXI_OK ==
                flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        REQUIRE(FLEXI_TYPE_FLOAT == flexi_cursor_type(&vcursor));
        REQUIRE(4 == flexi_cursor_width(&cursor));

        float f = 0.0f;
        REQUIRE(FLEXI_OK == flexi_cursor_f32(&vcursor, &f));
        REQUIRE_THAT(1.0f + float(i), WithinRel(f));
    }
}

/******************************************************************************/

static void
GetCursorThreeFloat64(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 27> s_data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, // Vector[0] (1.0)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, // Vector[1] (2.0)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x40, // Vector[2] (3.0)
        0x18, 0x57, 0x01                                // Root
    };

    auto buf = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&buf, &cursor));
}

TEST_CASE("flexi_cursor_typed_vector_data (Float64)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorThreeFloat64(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_FLOAT3 == flexi_cursor_type(&cursor));
    REQUIRE(8 == flexi_cursor_width(&cursor));
    REQUIRE(3 == flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    REQUIRE(FLEXI_TYPE_VECTOR_FLOAT3 == act_type);
    REQUIRE(8 == act_stride);
    REQUIRE(3 == act_count);

    const double *vec = static_cast<const double *>(act_data);
    REQUIRE_THAT(vec[0], WithinRel(1.0));
    REQUIRE_THAT(vec[1], WithinRel(2.0));
    REQUIRE_THAT(vec[2], WithinRel(3.0));
}

TEST_CASE("flexi_cursor_seek_vector_index (Float64)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorThreeFloat64(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_FLOAT3 == flexi_cursor_type(&cursor));
    REQUIRE(8 == flexi_cursor_width(&cursor));
    REQUIRE(3 == flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        REQUIRE(FLEXI_OK ==
                flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        REQUIRE(FLEXI_TYPE_FLOAT == flexi_cursor_type(&vcursor));
        REQUIRE(8 == flexi_cursor_width(&cursor));

        double f = 0.0;
        REQUIRE(FLEXI_OK == flexi_cursor_f64(&vcursor, &f));
        REQUIRE_THAT(1.0 + i, WithinRel(f));
    }
}

/******************************************************************************/

static void
GetCursorFiveBool(flexi_cursor_s &cursor)
{
    static std::array<uint8_t, 9> s_data = {
        0x05,            // Vector length
        0x01,            // Vector[0] (true)
        0x01,            // Vector[1] (true)
        0x00,            // Vector[2] (false)
        0x00,            // Vector[3] (false)
        0x01,            // Vector[4] (true)
        0x05, 0x90, 0x01 // Root
    };

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

TEST_CASE("flexi_cursor_typed_vector_data (Bool)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveBool(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_BOOL == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    REQUIRE(FLEXI_TYPE_VECTOR_BOOL == act_type);
    REQUIRE(1 == act_stride);
    REQUIRE(5 == act_count);

    const bool *vec = static_cast<const bool *>(act_data);
    REQUIRE(vec[0] == true);
    REQUIRE(vec[1] == true);
    REQUIRE(vec[2] == false);
    REQUIRE(vec[3] == false);
    REQUIRE(vec[4] == true);
}

TEST_CASE("flexi_cursor_seek_vector_index (Bool)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorFiveBool(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_BOOL == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    static constexpr std::array<bool, 5> s_expected = {
        true, true, false, false, true};

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        REQUIRE(FLEXI_OK ==
                flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        REQUIRE(FLEXI_TYPE_BOOL == flexi_cursor_type(&vcursor));
        REQUIRE(1 == flexi_cursor_width(&cursor));

        bool v = false;
        REQUIRE(FLEXI_OK == flexi_cursor_bool(&vcursor, &v));
        REQUIRE(s_expected[i] == v);
    }
}

/******************************************************************************/

static void
GetCursorKeys(flexi_cursor_s &cursor)
{
    static constexpr std::array<uint8_t, 19> s_data = {
        'f', 'i', 'r', 's', 't', '\0',      // First key
        's', 'e', 'c', 'o', 'n', 'd', '\0', // Second key
        0x02,                               // Vector length
        0x0E,                               // Vector[0] ("first")
        0x09,                               // Vector[1] ("second")
        0x02, 0x38, 0x01                    // Root
    };

    auto span = flexi_make_span(s_data.data(), s_data.size());
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
}

TEST_CASE("flexi_cursor_seek_vector_index (Key)", "[cursor_typed_vector]")
{
    flexi_cursor_s cursor{};
    GetCursorKeys(cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_KEY == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(2 == flexi_cursor_length(&cursor));

    std::array<const char *, 2> s_expected = {"first", "second"};

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        REQUIRE(FLEXI_OK ==
                flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        REQUIRE(FLEXI_TYPE_KEY == flexi_cursor_type(&vcursor));
        REQUIRE(1 == flexi_cursor_width(&cursor));

        const char *s = "";
        REQUIRE(FLEXI_OK == flexi_cursor_key(&vcursor, &s));
        REQUIRE_THAT(s_expected[i], Equals(s));
    }
}
