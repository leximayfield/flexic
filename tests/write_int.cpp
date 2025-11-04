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

struct WriteSintParams {
    int64_t value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

class WriteSintFixture : public WriteFixture,
                         public ::testing::WithParamInterface<WriteSintParams> {
};

TEST_P(WriteSintFixture, WriteAndRead) {
    auto [value, direct, ex_data, ex_width] = GetParam();

    if (direct == direct_e::direct) {
        ASSERT_TRUE(flexi_write_sint(&m_writer, value));
    } else {
        ASSERT_TRUE(flexi_write_indirect_sint(&m_writer, value));
    }

    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));
    ASSERT_EQ(FLEXI_TYPE_SINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    int64_t actual_value = 0;
    ASSERT_TRUE(flexi_cursor_sint(&cursor, &actual_value));
    ASSERT_EQ(value, actual_value);
}

INSTANTIATE_TEST_SUITE_P(
    WriteInt, WriteSintFixture,
    testing::Values(
        WriteSintParams{INT8_PATTERN, direct_e::direct, {0x88, 0x04, 0x01}, 1},
        WriteSintParams{
            INT16_PATTERN, direct_e::direct, {0x88, 0x99, 0x05, 0x02}, 2},
        WriteSintParams{INT32_PATTERN,
                        direct_e::direct,
                        {0x88, 0x99, 0xaa, 0xbb, 0x06, 0x04},
                        4},
        WriteSintParams{
            INT64_PATTERN,
            direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x07, 0x08},
            8}));

/******************************************************************************/

struct WriteUintParams {
    uint64_t value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

class WriteUintFixture : public WriteFixture,
                         public ::testing::WithParamInterface<WriteUintParams> {
};

TEST_P(WriteUintFixture, WriteAndRead) {
    auto [value, direct, ex_data, ex_width] = GetParam();

    if (direct == direct_e::direct) {
        ASSERT_TRUE(flexi_write_uint(&m_writer, value));
    } else {
        ASSERT_TRUE(flexi_write_indirect_uint(&m_writer, value));
    }

    ASSERT_TRUE(flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));
    ASSERT_EQ(FLEXI_TYPE_UINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    uint64_t actual_value = 0;
    ASSERT_TRUE(flexi_cursor_uint(&cursor, &actual_value));
    ASSERT_EQ(value, actual_value);
}

INSTANTIATE_TEST_SUITE_P(
    WriteUint, WriteUintFixture,
    testing::Values(
        WriteUintParams{UINT8_PATTERN, direct_e::direct, {0x88, 0x08, 0x01}, 1},
        WriteUintParams{
            UINT16_PATTERN, direct_e::direct, {0x99, 0x88, 0x09, 0x02}, 2},
        WriteUintParams{UINT32_PATTERN,
                        direct_e::direct,
                        {0xbb, 0xaa, 0x99, 0x88, 0x0a, 0x04},
                        4},
        WriteUintParams{
            UINT64_PATTERN,
            direct_e::direct,
            {0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x0b, 0x08},
            8}));
