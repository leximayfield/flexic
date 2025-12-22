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
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));
}

TEST(CursorTypedVector, Sint_Direct)
{
    flexi_cursor_s cursor{};
    GetCursorFiveSint(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_SINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    ASSERT_EQ(FLEXI_OK, flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    ASSERT_EQ(FLEXI_TYPE_VECTOR_SINT, act_type);
    ASSERT_EQ(4, act_stride);
    ASSERT_EQ(5, act_count);

    const int32_t *vec = static_cast<const int32_t *>(act_data);
    ASSERT_FLOAT_EQ(vec[0], 1);
    ASSERT_FLOAT_EQ(vec[1], 2);
    ASSERT_FLOAT_EQ(vec[2], 3);
    ASSERT_FLOAT_EQ(vec[3], 4);
    ASSERT_FLOAT_EQ(vec[4], 5);
}

TEST(CursorTypedVector, Sint_Seek)
{
    flexi_cursor_s cursor{};
    GetCursorFiveSint(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_SINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        ASSERT_EQ(FLEXI_OK,
            flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        ASSERT_EQ(FLEXI_TYPE_SINT, flexi_cursor_type(&vcursor));
        ASSERT_EQ(4, flexi_cursor_width(&cursor));

        int64_t v = 0;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&vcursor, &v));
        ASSERT_EQ(1 + int64_t(i), v);
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
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));
}

TEST(CursorTypedVector, Uint_Direct)
{
    flexi_cursor_s cursor{};
    GetCursorFiveUint(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_UINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    ASSERT_EQ(FLEXI_OK, flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    ASSERT_EQ(FLEXI_TYPE_VECTOR_UINT, act_type);
    ASSERT_EQ(4, act_stride);
    ASSERT_EQ(5, act_count);

    const uint32_t *vec = static_cast<const uint32_t *>(act_data);
    ASSERT_FLOAT_EQ(vec[0], 1);
    ASSERT_FLOAT_EQ(vec[1], 2);
    ASSERT_FLOAT_EQ(vec[2], 3);
    ASSERT_FLOAT_EQ(vec[3], 4);
    ASSERT_FLOAT_EQ(vec[4], 5);
}

TEST(CursorTypedVector, Uint_Seek)
{
    flexi_cursor_s cursor{};
    GetCursorFiveUint(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_UINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        ASSERT_EQ(FLEXI_OK,
            flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        ASSERT_EQ(FLEXI_TYPE_UINT, flexi_cursor_type(&vcursor));
        ASSERT_EQ(4, flexi_cursor_width(&cursor));

        uint64_t v = 0;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&vcursor, &v));
        ASSERT_EQ(1 + uint64_t(i), v);
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
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));
}

TEST(CursorTypedVector, Float32_Direct)
{
    flexi_cursor_s cursor{};
    GetCursorFiveFloat32(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    ASSERT_EQ(FLEXI_OK, flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT, act_type);
    ASSERT_EQ(4, act_stride);
    ASSERT_EQ(5, act_count);

    const float *vec = static_cast<const float *>(act_data);
    ASSERT_FLOAT_EQ(vec[0], 1.0f);
    ASSERT_FLOAT_EQ(vec[1], 2.0f);
    ASSERT_FLOAT_EQ(vec[2], 3.0f);
    ASSERT_FLOAT_EQ(vec[3], 4.0f);
    ASSERT_FLOAT_EQ(vec[4], 5.0f);
}

TEST(CursorTypedVector, Float32_Seek)
{
    flexi_cursor_s cursor{};
    GetCursorFiveFloat32(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        ASSERT_EQ(FLEXI_OK,
            flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&vcursor));
        ASSERT_EQ(4, flexi_cursor_width(&cursor));

        float f = 0.0f;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_f32(&vcursor, &f));
        ASSERT_FLOAT_EQ(1.0f + float(i), f);
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
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&buf, &cursor));
}

TEST(CursorTypedVector, Float64_Direct)
{
    flexi_cursor_s cursor{};
    GetCursorThreeFloat64(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT3, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_width(&cursor));
    ASSERT_EQ(3, flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    ASSERT_EQ(FLEXI_OK, flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT3, act_type);
    ASSERT_EQ(8, act_stride);
    ASSERT_EQ(3, act_count);

    const double *vec = static_cast<const double *>(act_data);
    ASSERT_FLOAT_EQ(vec[0], 1.0);
    ASSERT_FLOAT_EQ(vec[1], 2.0);
    ASSERT_FLOAT_EQ(vec[2], 3.0);
}

TEST(CursorTypedVector, Float64_Seek)
{
    flexi_cursor_s cursor{};
    GetCursorThreeFloat64(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT3, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_width(&cursor));
    ASSERT_EQ(3, flexi_cursor_length(&cursor));

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        ASSERT_EQ(FLEXI_OK,
            flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&vcursor));
        ASSERT_EQ(8, flexi_cursor_width(&cursor));

        double f = 0.0;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_f64(&vcursor, &f));
        ASSERT_EQ(1.0 + i, f);
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
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));
}

TEST(CursorTypedVector, Bool_Direct)
{
    flexi_cursor_s cursor{};
    GetCursorFiveBool(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_BOOL, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    const void *act_data = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_NULL;
    int act_stride = -1;
    flexi_ssize_t act_count = -1;

    ASSERT_EQ(FLEXI_OK, flexi_cursor_typed_vector_data(&cursor, &act_data,
                            &act_type, &act_stride, &act_count));
    ASSERT_EQ(FLEXI_TYPE_VECTOR_BOOL, act_type);
    ASSERT_EQ(1, act_stride);
    ASSERT_EQ(5, act_count);

    const bool *vec = static_cast<const bool *>(act_data);
    ASSERT_FLOAT_EQ(vec[0], true);
    ASSERT_FLOAT_EQ(vec[1], true);
    ASSERT_FLOAT_EQ(vec[2], false);
    ASSERT_FLOAT_EQ(vec[3], false);
    ASSERT_FLOAT_EQ(vec[4], true);
}

TEST(CursorTypedVector, Bool_Seek)
{
    flexi_cursor_s cursor{};
    GetCursorFiveBool(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_BOOL, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_width(&cursor));
    ASSERT_EQ(5, flexi_cursor_length(&cursor));

    static constexpr std::array<bool, 5> s_expected = {
        true, true, false, false, true};

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        ASSERT_EQ(FLEXI_OK,
            flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        ASSERT_EQ(FLEXI_TYPE_BOOL, flexi_cursor_type(&vcursor));
        ASSERT_EQ(1, flexi_cursor_width(&cursor));

        bool v = false;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&vcursor, &v));
        ASSERT_EQ(s_expected[i], v);
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
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));
}

TEST(CursorTypedVector, Key_Seek)
{
    flexi_cursor_s cursor{};
    GetCursorKeys(cursor);

    ASSERT_EQ(FLEXI_TYPE_VECTOR_KEY, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_width(&cursor));
    ASSERT_EQ(2, flexi_cursor_length(&cursor));

    std::array<const char *, 2> s_expected = {"first", "second"};

    flexi_ssize_t len = flexi_cursor_length(&cursor);
    for (flexi_ssize_t i = 0; i < len; i++) {
        flexi_cursor_s vcursor{};
        ASSERT_EQ(FLEXI_OK,
            flexi_cursor_seek_vector_index(&cursor, i, &vcursor));
        ASSERT_EQ(FLEXI_TYPE_KEY, flexi_cursor_type(&vcursor));
        ASSERT_EQ(1, flexi_cursor_width(&cursor));

        const char *s = "";
        ASSERT_EQ(FLEXI_OK, flexi_cursor_key(&vcursor, &s));
        ASSERT_STREQ(s_expected[i], s);
    }
}
