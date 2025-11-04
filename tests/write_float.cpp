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

#include <cmath>

/******************************************************************************/

struct WriteF32Params {
    float value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

class WriteF32Fixture : public WriteFixture,
                        public ::testing::WithParamInterface<WriteF32Params> {};

TEST_P(WriteF32Fixture, WriteAndRead) {
    auto [value, direct, ex_data, ex_width] = GetParam();

    if (direct == direct_e::direct) {
        ASSERT_TRUE(flexi_write_f32(&m_writer, value));
    } else {
        ASSERT_TRUE(flexi_write_indirect_f32(&m_writer, value));
    }

    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));
    if (direct == direct_e::direct) {
        ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&cursor));
    } else {
        ASSERT_EQ(FLEXI_TYPE_INDIRECT_FLOAT, flexi_cursor_type(&cursor));
    }
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    float actual_value = 0;
    ASSERT_TRUE(flexi_cursor_f32(&cursor, &actual_value));
    ASSERT_EQ(value, actual_value);
}

INSTANTIATE_TEST_SUITE_P(
    WriteFloat, WriteF32Fixture,
    testing::Values(WriteF32Params{PI_VALUE,
                                   direct_e::direct,
                                   {0xdb, 0x0f, 0x49, 0x40, 0x0e, 0x04},
                                   4},
                    WriteF32Params{PI_VALUE,
                                   direct_e::indirect,
                                   {0xdb, 0x0f, 0x49, 0x40, 0x04, 0x22, 0x01},
                                   4},
                    WriteF32Params{INFINITY,
                                   direct_e::direct,
                                   {0x00, 0x00, 0x80, 0x7f, 0x0e, 0x04},
                                   4},
                    WriteF32Params{INFINITY,
                                   direct_e::indirect,
                                   {0x00, 0x00, 0x80, 0x7f, 0x04, 0x22, 0x01},
                                   4}));

/******************************************************************************/

struct WriteF64Params {
    double value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

class WriteF64Fixture : public WriteFixture,
                        public ::testing::WithParamInterface<WriteF64Params> {};

TEST_P(WriteF64Fixture, WriteAndRead) {
    auto [value, direct, ex_data, ex_width] = GetParam();

    if (direct == direct_e::direct) {
        ASSERT_TRUE(flexi_write_f64(&m_writer, value));
    } else {
        ASSERT_TRUE(flexi_write_indirect_f64(&m_writer, value));
    }

    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));
    if (direct == direct_e::direct) {
        ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&cursor));
    } else {
        ASSERT_EQ(FLEXI_TYPE_INDIRECT_FLOAT, flexi_cursor_type(&cursor));
    }
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    double actual_value = 0;
    ASSERT_TRUE(flexi_cursor_f64(&cursor, &actual_value));
    ASSERT_EQ(value, actual_value);
}

INSTANTIATE_TEST_SUITE_P(
    WriteFloat, WriteF64Fixture,
    testing::Values(WriteF64Params{PI_VALUE,
                                   direct_e::direct,
                                   {0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09,
                                    0x40, 0x0f, 0x08},
                                   8},
                    WriteF64Params{PI_VALUE,
                                   direct_e::indirect,
                                   {0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09,
                                    0x40, 0x08, 0x23, 0x01},
                                   8},
                    WriteF64Params{INFINITY,
                                   direct_e::direct,
                                   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
                                    0x7f, 0x0f, 0x08},
                                   8},
                    WriteF64Params{INFINITY,
                                   direct_e::indirect,
                                   {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
                                    0x7f, 0x08, 0x23, 0x01},
                                   8}));

/******************************************************************************/

struct WriteVecF32Params {
    std::vector<float> ex_values;
    std::vector<uint8_t> ex_data;
    flexi_type_e ex_type;
    int ex_width;
};

class WriteVecF32Fixture
    : public WriteFixture,
      public ::testing::WithParamInterface<WriteVecF32Params> {};

TEST_P(WriteVecF32Fixture, WriteAndRead) {
    auto [ex_values, ex_data, ex_type, ex_width] = GetParam();

    ASSERT_TRUE(
        flexi_write_vector_f32(&m_writer, ex_values.data(), ex_values.size()));
    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(ex_type, flexi_cursor_type(&cursor));
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    size_t len = 0;
    ASSERT_TRUE(flexi_cursor_length(&cursor, &len));

    const void *out_ptr = nullptr;
    ASSERT_TRUE(flexi_cursor_typed_vector_data(&cursor, &out_ptr));

    auto out_values = static_cast<const float *>(out_ptr);
    for (size_t i = 0; i < ex_values.size(); i++) {
        ASSERT_FLOAT_EQ(ex_values[i], out_values[i]);
    }
}

INSTANTIATE_TEST_SUITE_P( //
    WriteFloat, WriteVecF32Fixture,
    testing::Values( //
        WriteVecF32Params{{1.0, -2.0},
                          {0x00, 0x00, 0x80, 0x3f, // Vector[0]
                           0x00, 0x00, 0x00, 0xc0, // Vector[1]
                           0x08, 0x4a, 0x01},      // Root
                          FLEXI_TYPE_VECTOR_FLOAT2,
                          4},
        WriteVecF32Params{{1.0, -2.0, PI_VALUE},
                          {0x00, 0x00, 0x80, 0x3f, // Vector[0]
                           0x00, 0x00, 0x00, 0xc0, // Vector[1]
                           0xdb, 0x0f, 0x49, 0x40, // Vector[2]
                           0x0c, 0x56, 0x01},      // Root
                          FLEXI_TYPE_VECTOR_FLOAT3,
                          4},
        WriteVecF32Params{{1.0, -2.0, PI_VALUE, -1e4},
                          {0x00, 0x00, 0x80, 0x3f, // Vector[0]
                           0x00, 0x00, 0x00, 0xc0, // Vector[1]
                           0xdb, 0x0f, 0x49, 0x40, // Vector[2]
                           0x00, 0x40, 0x1c, 0xc6, // Vector[3]
                           0x10, 0x62, 0x01},      // Root
                          FLEXI_TYPE_VECTOR_FLOAT4,
                          4},
        WriteVecF32Params{{1.0, -2.0, PI_VALUE, -1e4, 1e-5},
                          {0x05, 0x00, 0x00, 0x00, // Vector length
                           0x00, 0x00, 0x80, 0x3f, // Vector[0]
                           0x00, 0x00, 0x00, 0xc0, // Vector[1]
                           0xdb, 0x0f, 0x49, 0x40, // Vector[2]
                           0x00, 0x40, 0x1c, 0xc6, // Vector[3]
                           0xac, 0xc5, 0x27, 0x37, // Vector[4]
                           0x14, 0x36, 0x01},      // Root
                          FLEXI_TYPE_VECTOR_FLOAT,
                          4}));

/******************************************************************************/

struct WriteVecF64Params {
    std::vector<double> ex_values;
    std::vector<uint8_t> ex_data;
    flexi_type_e ex_type;
    int ex_width;
};

class WriteVecF64Fixture
    : public WriteFixture,
      public ::testing::WithParamInterface<WriteVecF64Params> {};

TEST_P(WriteVecF64Fixture, WriteAndRead) {
    auto [ex_values, ex_data, ex_type, ex_width] = GetParam();

    ASSERT_TRUE(
        flexi_write_vector_f64(&m_writer, ex_values.data(), ex_values.size()));
    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(ex_type, flexi_cursor_type(&cursor));
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    size_t len = 0;
    ASSERT_TRUE(flexi_cursor_length(&cursor, &len));

    const void *out_ptr = nullptr;
    ASSERT_TRUE(flexi_cursor_typed_vector_data(&cursor, &out_ptr));

    auto out_values = static_cast<const double *>(out_ptr);
    for (size_t i = 0; i < ex_values.size(); i++) {
        ASSERT_FLOAT_EQ(ex_values[i], out_values[i]);
    }
}

INSTANTIATE_TEST_SUITE_P( //
    WriteFloat, WriteVecF64Fixture,
    testing::Values( //
        WriteVecF64Params{
            {1.0, -2.0},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, // Vector[0]
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
             0x10, 0x4b, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT2,
            8},
        WriteVecF64Params{
            {1.0, -2.0, PI_VALUE},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, // Vector[0]
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
             0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, // Vector[2]
             0x18, 0x57, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT3,
            8},
        WriteVecF64Params{
            {1.0, -2.0, PI_VALUE, -1e4},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, // Vector[0]
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
             0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, // Vector[2]
             0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xc3, 0xc0, // Vector[3]
             0x20, 0x63, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT4,
            8},
        WriteVecF64Params{
            {1.0, -2.0, PI_VALUE, -1e4, 1e-5},
            {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector length
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, // Vector[0]
             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, // Vector[1]
             0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40, // Vector[2]
             0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0xc3, 0xc0, // Vector[3]
             0xf1, 0x68, 0xe3, 0x88, 0xb5, 0xf8, 0xe4, 0x3e, // Vector[4]
             0x28, 0x37, 0x01},                              // Root
            FLEXI_TYPE_VECTOR_FLOAT,
            8}));
