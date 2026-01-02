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

#include "catch2/generators/catch_generators.hpp"
#include "tests.hpp"

#include <cmath>

/******************************************************************************/

struct WriteF32Params {
    float value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

TEST_CASE("Write Float32", "[write_float]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    WriteF32Params params = GENERATE( //
        WriteF32Params{PI_VALUE_FLT, direct_e::direct,
            {0xdb, 0x0f, 0x49, 0x40, 0x0e, 0x04}, 4},
        WriteF32Params{PI_VALUE_FLT, direct_e::indirect,
            {0xdb, 0x0f, 0x49, 0x40, 0x04, 0x22, 0x01}, 4},
        WriteF32Params{INFINITY, direct_e::direct,
            {0x00, 0x00, 0x80, 0x7f, 0x0e, 0x04}, 4},
        WriteF32Params{INFINITY, direct_e::indirect,
            {0x00, 0x00, 0x80, 0x7f, 0x04, 0x22, 0x01}, 4});

    if (params.direct == direct_e::direct) {
        REQUIRE(FLEXI_OK == flexi_write_f32(fwriter, NULL, params.value));
    } else {
        REQUIRE(FLEXI_OK ==
                flexi_write_indirect_f32(fwriter, NULL, params.value));
    }

    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    writer.AssertData(params.ex_data);
    TestStream &actual = writer.GetActual();

    flexi_ssize_t offset = 0;
    REQUIRE(actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto span = flexi_make_span(actual.DataAt(0), offset);
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
    if (params.direct == direct_e::direct) {
        REQUIRE(FLEXI_TYPE_FLOAT == flexi_cursor_type(&cursor));
    } else {
        REQUIRE(FLEXI_TYPE_INDIRECT_FLOAT == flexi_cursor_type(&cursor));
    }
    REQUIRE(params.ex_width == flexi_cursor_width(&cursor));

    float actual_value = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_f32(&cursor, &actual_value));
    REQUIRE_THAT(params.value, WithinRel(actual_value));
}

/******************************************************************************/

struct WriteF64Params {
    double value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

TEST_CASE("Write Float64", "[write_float]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    WriteF64Params params = GENERATE( //
        WriteF64Params{PI_VALUE_DBL, direct_e::direct,
            {0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, 0x0f, 0x08}, 8},
        WriteF64Params{PI_VALUE_DBL, direct_e::indirect,
            {0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, 0x08, 0x23, 0x01},
            8},
        WriteF64Params{INFINITY, direct_e::direct,
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x0f, 0x08}, 8},
        WriteF64Params{INFINITY, direct_e::indirect,
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x08, 0x23, 0x01},
            8});

    if (params.direct == direct_e::direct) {
        REQUIRE(FLEXI_OK == flexi_write_f64(fwriter, NULL, params.value));
    } else {
        REQUIRE(FLEXI_OK ==
                flexi_write_indirect_f64(fwriter, NULL, params.value));
    }

    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    writer.AssertData(params.ex_data);
    TestStream &actual = writer.GetActual();

    flexi_ssize_t offset = 0;
    REQUIRE(actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto span = flexi_make_span(actual.DataAt(0), offset);
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
    if (params.direct == direct_e::direct) {
        REQUIRE(FLEXI_TYPE_FLOAT == flexi_cursor_type(&cursor));
    } else {
        REQUIRE(FLEXI_TYPE_INDIRECT_FLOAT == flexi_cursor_type(&cursor));
    }
    REQUIRE(params.ex_width == flexi_cursor_width(&cursor));

    double actual_value = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_f64(&cursor, &actual_value));
    REQUIRE(params.value == actual_value);
}

/******************************************************************************/

struct WriteVecF32Params {
    std::vector<float> ex_values;
    std::vector<uint8_t> ex_data;
    flexi_type_e ex_type;
    int ex_width;
};

TEST_CASE("Write Vector of Float32", "[write_float]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    WriteVecF32Params params = GENERATE( //
        WriteVecF32Params{{1.0, -2.0},
            {0x00, 0x00, 0x80, 0x3f,    // Vector[0]
                0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0x08, 0x4a, 0x01},      // Root
            FLEXI_TYPE_VECTOR_FLOAT2, 4},
        WriteVecF32Params{{1.0, -2.0, PI_VALUE_FLT},
            {0x00, 0x00, 0x80, 0x3f,    // Vector[0]
                0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0xdb, 0x0f, 0x49, 0x40, // Vector[2]
                0x0c, 0x56, 0x01},      // Root
            FLEXI_TYPE_VECTOR_FLOAT3, 4},
        WriteVecF32Params{{1.0, -2.0, PI_VALUE_FLT, -1e4},
            {0x00, 0x00, 0x80, 0x3f,    // Vector[0]
                0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0xdb, 0x0f, 0x49, 0x40, // Vector[2]
                0x00, 0x40, 0x1c, 0xc6, // Vector[3]
                0x10, 0x62, 0x01},      // Root
            FLEXI_TYPE_VECTOR_FLOAT4, 4},
        WriteVecF32Params{{1.0, -2.0, PI_VALUE_FLT, -1e4, 1e-5},
            {0x05, 0x00, 0x00, 0x00,    // Vector length
                0x00, 0x00, 0x80, 0x3f, // Vector[0]
                0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0xdb, 0x0f, 0x49, 0x40, // Vector[2]
                0x00, 0x40, 0x1c, 0xc6, // Vector[3]
                0xac, 0xc5, 0x27, 0x37, // Vector[4]
                0x14, 0x36, 0x01},      // Root
            FLEXI_TYPE_VECTOR_FLOAT, 4});

    REQUIRE(FLEXI_OK == flexi_write_typed_vector(fwriter, NULL,
                            params.ex_values.data(), params.ex_values.size()));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    writer.AssertData(params.ex_data);
    TestStream &actual = writer.GetActual();

    flexi_ssize_t offset = 0;
    REQUIRE(actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto span = flexi_make_span(actual.DataAt(0), offset);
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    REQUIRE(params.ex_type == flexi_cursor_type(&cursor));
    REQUIRE(params.ex_width == flexi_cursor_width(&cursor));
    REQUIRE(params.ex_values.size() == flexi_cursor_length(&cursor));

    const void *out_ptr = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_INVALID;
    int act_width = -1;
    flexi_ssize_t act_count = -1;
    REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &out_ptr,
                            &act_type, &act_width, &act_count));

    auto out_values = static_cast<const float *>(out_ptr);
    for (size_t i = 0; i < params.ex_values.size(); i++) {
        REQUIRE_THAT(params.ex_values[i], WithinRel(out_values[i]));
    }
}

/******************************************************************************/

struct WriteVecF64Params {
    std::vector<double> ex_values;
    std::vector<uint8_t> ex_data;
    flexi_type_e ex_type;
    int ex_width;
};

TEST_CASE("Write Vector of Float64", "[write_float]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    WriteVecF64Params params = GENERATE( //
        WriteVecF64Params{{1.0, -2.0},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f,    // Vector[0]
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0x10, 0x4b, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT2, 8},
        WriteVecF64Params{{1.0, -2.0, PI_VALUE_DBL},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f,    // Vector[0]
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, // Vector[2]
                0x18, 0x57, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT3, 8},
        WriteVecF64Params{{1.0, -2.0, PI_VALUE_DBL, -1e4},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f,    // Vector[0]
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, // Vector[2]
                0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xc3, 0xc0, // Vector[3]
                0x20, 0x63, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT4, 8},
        WriteVecF64Params{{1.0, -2.0, PI_VALUE_DBL, -1e4, 1e-5},
            {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    // Vector length
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, // Vector[0]
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
                0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, // Vector[2]
                0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xc3, 0xc0, // Vector[3]
                0xf1, 0x68, 0xe3, 0x88, 0xb5, 0xf8, 0xe4, 0x3e, // Vector[4]
                0x28, 0x37, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT, 8});

    REQUIRE(FLEXI_OK == flexi_write_typed_vector(fwriter, NULL,
                            params.ex_values.data(), params.ex_values.size()));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    writer.AssertData(params.ex_data);
    TestStream &actual = writer.GetActual();

    flexi_ssize_t offset = 0;
    REQUIRE(actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto span = flexi_make_span(actual.DataAt(0), offset);
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    REQUIRE(params.ex_type == flexi_cursor_type(&cursor));
    REQUIRE(params.ex_width == flexi_cursor_width(&cursor));
    REQUIRE(params.ex_values.size() == flexi_cursor_length(&cursor));

    const void *out_ptr = nullptr;
    flexi_type_e act_type = FLEXI_TYPE_INVALID;
    int act_width = -1;
    flexi_ssize_t act_count = -1;
    REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &out_ptr,
                            &act_type, &act_width, &act_count));

    auto out_values = static_cast<const double *>(out_ptr);
    for (size_t i = 0; i < params.ex_values.size(); i++) {
        REQUIRE_THAT(params.ex_values[i], WithinRel(out_values[i]));
    }
}
