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
                         public testing::WithParamInterface<WriteSintParams> {};

TEST_P(WriteSintFixture, WriteAndRead)
{
    auto [value, direct, ex_data, ex_width] = this->GetParam();

    if (direct == direct_e::direct) {
        ASSERT_EQ(FLEXI_OK, flexi_write_sint(&m_writer, value));
    } else {
        ASSERT_EQ(FLEXI_OK, flexi_write_indirect_sint(&m_writer, value));
    }

    ASSERT_EQ(FLEXI_OK, flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
    ASSERT_EQ(FLEXI_TYPE_SINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    int64_t actual_value = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&cursor, &actual_value));
    ASSERT_EQ(value, actual_value);
}

INSTANTIATE_TEST_SUITE_P(WriteInt, WriteSintFixture,
    testing::Values(
        WriteSintParams{INT8_PATTERN, direct_e::direct, {0x88, 0x04, 0x01}, 1},
        WriteSintParams{
            INT16_PATTERN, direct_e::direct, {0x88, 0x99, 0x05, 0x02}, 2},
        WriteSintParams{INT32_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0x06, 0x04}, 4},
        WriteSintParams{INT64_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x07, 0x08}, 8}));

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

TEST_P(WriteUintFixture, WriteAndRead)
{
    auto [value, direct, ex_data, ex_width] = this->GetParam();

    if (direct == direct_e::direct) {
        ASSERT_EQ(FLEXI_OK, flexi_write_uint(&m_writer, value));
    } else {
        ASSERT_EQ(FLEXI_OK, flexi_write_indirect_uint(&m_writer, value));
    }

    ASSERT_EQ(FLEXI_OK, flexi_write_finalize(&m_writer));

    AssertData(ex_data);

    size_t offset = 0;
    ASSERT_TRUE(m_actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));
    ASSERT_EQ(FLEXI_TYPE_UINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(ex_width, flexi_cursor_width(&cursor));

    uint64_t actual_value = 0;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&cursor, &actual_value));
    ASSERT_EQ(value, actual_value);
}

INSTANTIATE_TEST_SUITE_P(WriteUint, WriteUintFixture,
    testing::Values(
        WriteUintParams{UINT8_PATTERN, direct_e::direct, {0x88, 0x08, 0x01}, 1},
        WriteUintParams{
            UINT16_PATTERN, direct_e::direct, {0x88, 0x99, 0x09, 0x02}, 2},
        WriteUintParams{UINT32_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0x0a, 0x04}, 4},
        WriteUintParams{UINT64_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x0b, 0x08}, 8}));

/******************************************************************************/

template<typename T> struct WriteIntVectorParams {
    std::vector<T> ex_values;
    std::vector<uint8_t> ex_data;
    flexi_type_e ex_tyoe;
};

template<typename T> class WriteIntVectorFixture
    : public WriteFixture,
      public ::testing::WithParamInterface<WriteIntVectorParams<T>> {
public:
    void Run()
    {
        auto [ex_values, ex_data, ex_tyoe] = this->GetParam();

        ASSERT_EQ(FLEXI_OK, flexi_write_typed_vector<T>(&m_writer,
                                ex_values.data(), ex_values.size()));
        ASSERT_EQ(FLEXI_OK, flexi_write_finalize(&m_writer));

        AssertData(ex_data);

        flexi_cursor_s cursor{};
        GetCursor(&cursor);

        ASSERT_EQ(ex_tyoe, flexi_cursor_type(&cursor));
        ASSERT_EQ(sizeof(T), flexi_cursor_width(&cursor));
        ASSERT_EQ(ex_values.size(), flexi_cursor_length(&cursor));
    }
};

using WriteSint8VectorFixture = WriteIntVectorFixture<int8_t>;

TEST_P(WriteSint8VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteSint8Vector, WriteSint8VectorFixture,
    testing::Values( //
        WriteIntVectorParams<int8_t>{{-1, -2},
            {
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0x02, 0x40, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        WriteIntVectorParams<int8_t>{{-1, -2, -3},
            {
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0xfd,            // Vector[2]
                0x03, 0x4c, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        WriteIntVectorParams<int8_t>{{-1, -2, -3, -4},
            {
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0xfd,            // Vector[2]
                0xfc,            // Vector[3]
                0x04, 0x58, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        WriteIntVectorParams<int8_t>{{-1, -2, -3, -4, -5},
            {
                0x05,            // Vector length
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0xfd,            // Vector[2]
                0xfc,            // Vector[3]
                0xfb,            // Vector[4]
                0x05, 0x2c, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT}));

using WriteSint16VectorFixture = WriteIntVectorFixture<int16_t>;

TEST_P(WriteSint16VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteSint16Vector, WriteSint16VectorFixture,
    testing::Values( //
        WriteIntVectorParams<int16_t>{{-1, -2},
            {
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0x04, 0x41, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        WriteIntVectorParams<int16_t>{{-1, -2, -3},
            {
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0xfd, 0xff,      // Vector[2]
                0x06, 0x4d, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        WriteIntVectorParams<int16_t>{{-1, -2, -3, -4},
            {
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0xfd, 0xff,      // Vector[2]
                0xfc, 0xff,      // Vector[3]
                0x08, 0x59, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        WriteIntVectorParams<int16_t>{{-1, -2, -3, -4, -5},
            {
                0x05, 0x00,      // Vector length
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0xfd, 0xff,      // Vector[2]
                0xfc, 0xff,      // Vector[3]
                0xfb, 0xff,      // Vector[4]
                0x0a, 0x2d, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT}));

using WriteSint32VectorFixture = WriteIntVectorFixture<int32_t>;

TEST_P(WriteSint32VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteSint32Vector, WriteSint32VectorFixture,
    testing::Values( //
        WriteIntVectorParams<int32_t>{{-1, -2},
            {
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0x08, 0x42, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        WriteIntVectorParams<int32_t>{{-1, -2, -3},
            {
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, // Vector[2]
                0x0c, 0x4e, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        WriteIntVectorParams<int32_t>{{-1, -2, -3, -4},
            {
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, // Vector[3]
                0x10, 0x5a, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        WriteIntVectorParams<int32_t>{{-1, -2, -3, -4, -5},
            {
                0x05, 0x00, 0x00, 0x00, // Vector length
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, // Vector[3]
                0xfb, 0xff, 0xff, 0xff, // Vector[4]
                0x14, 0x2e, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT}));

using WriteSint64VectorFixture = WriteIntVectorFixture<int64_t>;

TEST_P(WriteSint64VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteSint64Vector, WriteSint64VectorFixture,
    testing::Values( //
        WriteIntVectorParams<int64_t>{{-1, -2},
            {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0x10, 0x43, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        WriteIntVectorParams<int64_t>{{-1, -2, -3},
            {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[2]
                0x18, 0x4f, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        WriteIntVectorParams<int64_t>{{-1, -2, -3, -4},
            {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[3]
                0x20, 0x5b, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        WriteIntVectorParams<int64_t>{{-1, -2, -3, -4, -5},
            {
                0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector length
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[3]
                0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[4]
                0x28, 0x2f, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT}));

/******************************************************************************/

using WriteUint8VectorFixture = WriteIntVectorFixture<uint8_t>;

TEST_P(WriteUint8VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteUint8Vector, WriteUint8VectorFixture,
    testing::Values( //
        WriteIntVectorParams<uint8_t>{{1, 2},
            {
                0x01,            // Vector[0]
                0x02,            // Vector[1]
                0x02, 0x44, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        WriteIntVectorParams<uint8_t>{{1, 2, 3},
            {
                0x01,            // Vector[0]
                0x02,            // Vector[1]
                0x03,            // Vector[2]
                0x03, 0x50, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        WriteIntVectorParams<uint8_t>{{1, 2, 3, 4},
            {
                0x01,            // Vector[1]
                0x02,            // Vector[1]
                0x03,            // Vector[2]
                0x04,            // Vector[3]
                0x04, 0x5c, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        WriteIntVectorParams<uint8_t>{{1, 2, 3, 4, 5},
            {
                0x05,            // Vector length
                0x01,            // Vector[0]
                0x02,            // Vector[1]
                0x03,            // Vector[2]
                0x04,            // Vector[3]
                0x05,            // Vector[4]
                0x05, 0x30, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT}));

using WriteUint16VectorFixture = WriteIntVectorFixture<uint16_t>;

TEST_P(WriteUint16VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteUint16Vector, WriteUint16VectorFixture,
    testing::Values( //
        WriteIntVectorParams<uint16_t>{{1, 2},
            {
                0x01, 0x00,      // Vector[0]
                0x02, 0x00,      // Vector[1]
                0x04, 0x45, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        WriteIntVectorParams<uint16_t>{{1, 2, 3},
            {
                0x01, 0x00,      // Vector[0]
                0x02, 0x00,      // Vector[1]
                0x03, 0x00,      // Vector[2]
                0x06, 0x51, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        WriteIntVectorParams<uint16_t>{{1, 2, 3, 4},
            {
                0x01, 0x00,      // Vector[1]
                0x02, 0x00,      // Vector[1]
                0x03, 0x00,      // Vector[2]
                0x04, 0x00,      // Vector[3]
                0x08, 0x5d, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        WriteIntVectorParams<uint16_t>{{1, 2, 3, 4, 5},
            {
                0x05, 0x00,      // Vector length
                0x01, 0x00,      // Vector[0]
                0x02, 0x00,      // Vector[1]
                0x03, 0x00,      // Vector[2]
                0x04, 0x00,      // Vector[3]
                0x05, 0x00,      // Vector[4]
                0x0a, 0x31, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT}));

using WriteUint32VectorFixture = WriteIntVectorFixture<uint32_t>;

TEST_P(WriteUint32VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteUint32Vector, WriteUint32VectorFixture,
    testing::Values( //
        WriteIntVectorParams<uint32_t>{{1, 2},
            {
                0x01, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x08, 0x46, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        WriteIntVectorParams<uint32_t>{{1, 2, 3},
            {
                0x01, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, // Vector[2]
                0x0c, 0x52, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        WriteIntVectorParams<uint32_t>{{1, 2, 3, 4},
            {
                0x01, 0x00, 0x00, 0x00, // Vector[1]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, // Vector[3]
                0x10, 0x5e, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        WriteIntVectorParams<uint32_t>{{1, 2, 3, 4, 5},
            {
                0x05, 0x00, 0x00, 0x00, // Vector length
                0x01, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, // Vector[3]
                0x05, 0x00, 0x00, 0x00, // Vector[4]
                0x14, 0x32, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT}));

using WriteUint64VectorFixture = WriteIntVectorFixture<uint64_t>;

TEST_P(WriteUint64VectorFixture, WriteAndRead) { Run(); }

INSTANTIATE_TEST_SUITE_P(WriteUint64Vector, WriteUint64VectorFixture,
    testing::Values( //
        WriteIntVectorParams<uint64_t>{{1, 2},
            {
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x10, 0x47, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        WriteIntVectorParams<uint64_t>{{1, 2, 3},
            {
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[2]
                0x18, 0x53, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        WriteIntVectorParams<uint64_t>{{1, 2, 3, 4},
            {
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[3]
                0x20, 0x5f, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        WriteIntVectorParams<uint64_t>{{1, 2, 3, 4, 5},
            {
                0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector length
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[3]
                0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[4]
                0x28, 0x33, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT}));
