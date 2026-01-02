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

#include <catch2/generators/catch_generators.hpp>

/******************************************************************************/

struct WriteSintParams {
    int64_t value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

TEST_CASE("Write Sint", "[write_int]")
{
    const WriteSintParams params = GENERATE(
        WriteSintParams{INT8_PATTERN, direct_e::direct, {0x88, 0x04, 0x01}, 1},
        WriteSintParams{
            INT16_PATTERN, direct_e::direct, {0x88, 0x99, 0x05, 0x02}, 2},
        WriteSintParams{INT32_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0x06, 0x04}, 4},
        WriteSintParams{INT64_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x07, 0x08}, 8});

    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    if (params.direct == direct_e::direct) {
        REQUIRE(FLEXI_OK == flexi_write_sint(fwriter, nullptr, params.value));
    } else {
        REQUIRE(FLEXI_OK ==
                flexi_write_indirect_sint(fwriter, nullptr, params.value));
    }

    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    writer.AssertData(params.ex_data);
    TestStream &actual = writer.GetActual();

    flexi_ssize_t offset = 0;
    REQUIRE(actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto span = flexi_make_span(actual.DataAt(0), offset);
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
    REQUIRE(FLEXI_TYPE_SINT == flexi_cursor_type(&cursor));
    REQUIRE(params.ex_width == flexi_cursor_width(&cursor));

    int64_t actual_value = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_sint(&cursor, &actual_value));
    REQUIRE(params.value == actual_value);
}

/******************************************************************************/

struct WriteUintParams {
    uint64_t value;
    direct_e direct;
    std::vector<uint8_t> ex_data;
    int ex_width;
};

TEST_CASE("Write Uint", "[write_int]")
{
    const WriteUintParams params = GENERATE(
        WriteUintParams{UINT8_PATTERN, direct_e::direct, {0x88, 0x08, 0x01}, 1},
        WriteUintParams{
            UINT16_PATTERN, direct_e::direct, {0x88, 0x99, 0x09, 0x02}, 2},
        WriteUintParams{UINT32_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0x0a, 0x04}, 4},
        WriteUintParams{UINT64_PATTERN, direct_e::direct,
            {0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x0b, 0x08}, 8});

    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    if (params.direct == direct_e::direct) {
        REQUIRE(FLEXI_OK == flexi_write_uint(fwriter, nullptr, params.value));
    } else {
        REQUIRE(FLEXI_OK ==
                flexi_write_indirect_uint(fwriter, nullptr, params.value));
    }

    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    writer.AssertData(params.ex_data);
    TestStream &actual = writer.GetActual();

    flexi_ssize_t offset = 0;
    REQUIRE(actual.Tell(&offset));

    flexi_cursor_s cursor{};
    auto span = flexi_make_span(actual.DataAt(0), offset);
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));
    REQUIRE(FLEXI_TYPE_UINT == flexi_cursor_type(&cursor));
    REQUIRE(params.ex_width == flexi_cursor_width(&cursor));

    uint64_t actual_value = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_uint(&cursor, &actual_value));
    REQUIRE(params.value == actual_value);
}

/******************************************************************************/

template<typename T> struct WriteIntVectorParams {
    std::vector<T> ex_values;
    std::vector<uint8_t> ex_data;
    flexi_type_e ex_tyoe;
};

template<typename T> static void
RunTypedVector(WriteIntVectorParams<T> &params)
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    REQUIRE(FLEXI_OK == flexi_write_typed_vector<T>(fwriter, nullptr,
                            params.ex_values.data(), params.ex_values.size()));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    writer.AssertData(params.ex_data);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(params.ex_tyoe == flexi_cursor_type(&cursor));
    REQUIRE(sizeof(T) == flexi_cursor_width(&cursor));
    REQUIRE(params.ex_values.size() == flexi_cursor_length(&cursor));
}

TEST_CASE("Write Typed Vector (Int8)", "[write_int]")
{
    using params_t = WriteIntVectorParams<int8_t>;
    params_t params = GENERATE( //
        params_t{{-1, -2},
            {
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0x02, 0x40, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        params_t{{-1, -2, -3},
            {
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0xfd,            // Vector[2]
                0x03, 0x4c, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        params_t{{-1, -2, -3, -4},
            {
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0xfd,            // Vector[2]
                0xfc,            // Vector[3]
                0x04, 0x58, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        params_t{{-1, -2, -3, -4, -5},
            {
                0x05,            // Vector length
                0xff,            // Vector[0]
                0xfe,            // Vector[1]
                0xfd,            // Vector[2]
                0xfc,            // Vector[3]
                0xfb,            // Vector[4]
                0x05, 0x2c, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT});

    RunTypedVector<int8_t>(params);
}

TEST_CASE("Write Typed Vector (Int16)", "[write_int]")
{
    using params_t = WriteIntVectorParams<int16_t>;
    params_t params = GENERATE( //
        params_t{{-1, -2},
            {
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0x04, 0x41, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        params_t{{-1, -2, -3},
            {
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0xfd, 0xff,      // Vector[2]
                0x06, 0x4d, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        params_t{{-1, -2, -3, -4},
            {
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0xfd, 0xff,      // Vector[2]
                0xfc, 0xff,      // Vector[3]
                0x08, 0x59, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        params_t{{-1, -2, -3, -4, -5},
            {
                0x05, 0x00,      // Vector length
                0xff, 0xff,      // Vector[0]
                0xfe, 0xff,      // Vector[1]
                0xfd, 0xff,      // Vector[2]
                0xfc, 0xff,      // Vector[3]
                0xfb, 0xff,      // Vector[4]
                0x0a, 0x2d, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_SINT});

    RunTypedVector<int16_t>(params);
}

TEST_CASE("Write Typed Vector (Int32)", "[write_int]")
{
    using params_t = WriteIntVectorParams<int32_t>;
    params_t params = GENERATE( //
        params_t{{-1, -2},
            {
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0x08, 0x42, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        params_t{{-1, -2, -3},
            {
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, // Vector[2]
                0x0c, 0x4e, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        params_t{{-1, -2, -3, -4},
            {
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, // Vector[3]
                0x10, 0x5a, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        params_t{{-1, -2, -3, -4, -5},
            {
                0x05, 0x00, 0x00, 0x00, // Vector length
                0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, // Vector[3]
                0xfb, 0xff, 0xff, 0xff, // Vector[4]
                0x14, 0x2e, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_SINT});

    RunTypedVector<int32_t>(params);
}

TEST_CASE("Write Typed Vector (Int64)", "[write_int]")
{
    using params_t = WriteIntVectorParams<int64_t>;
    params_t params = GENERATE( //
        params_t{{-1, -2},
            {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0x10, 0x43, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT2},
        params_t{{-1, -2, -3},
            {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[2]
                0x18, 0x4f, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT3},
        params_t{{-1, -2, -3, -4},
            {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[3]
                0x20, 0x5b, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT4},
        params_t{{-1, -2, -3, -4, -5},
            {
                0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector length
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[0]
                0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[1]
                0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[2]
                0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[3]
                0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Vector[4]
                0x28, 0x2f, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_SINT});

    RunTypedVector<int64_t>(params);
}

TEST_CASE("Write Typed Vector (Uint8)", "[write_int]")
{
    using params_t = WriteIntVectorParams<uint8_t>;
    params_t params = GENERATE( //
        params_t{{1, 2},
            {
                0x01,            // Vector[0]
                0x02,            // Vector[1]
                0x02, 0x44, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        params_t{{1, 2, 3},
            {
                0x01,            // Vector[0]
                0x02,            // Vector[1]
                0x03,            // Vector[2]
                0x03, 0x50, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        params_t{{1, 2, 3, 4},
            {
                0x01,            // Vector[1]
                0x02,            // Vector[1]
                0x03,            // Vector[2]
                0x04,            // Vector[3]
                0x04, 0x5c, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        params_t{{1, 2, 3, 4, 5},
            {
                0x05,            // Vector length
                0x01,            // Vector[0]
                0x02,            // Vector[1]
                0x03,            // Vector[2]
                0x04,            // Vector[3]
                0x05,            // Vector[4]
                0x05, 0x30, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT});

    RunTypedVector<uint8_t>(params);
}

TEST_CASE("Write Typed Vector (Uint16)", "[write_int]")
{
    using params_t = WriteIntVectorParams<uint16_t>;
    params_t params = GENERATE( //
        params_t{{1, 2},
            {
                0x01, 0x00,      // Vector[0]
                0x02, 0x00,      // Vector[1]
                0x04, 0x45, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        params_t{{1, 2, 3},
            {
                0x01, 0x00,      // Vector[0]
                0x02, 0x00,      // Vector[1]
                0x03, 0x00,      // Vector[2]
                0x06, 0x51, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        params_t{{1, 2, 3, 4},
            {
                0x01, 0x00,      // Vector[1]
                0x02, 0x00,      // Vector[1]
                0x03, 0x00,      // Vector[2]
                0x04, 0x00,      // Vector[3]
                0x08, 0x5d, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        params_t{{1, 2, 3, 4, 5},
            {
                0x05, 0x00,      // Vector length
                0x01, 0x00,      // Vector[0]
                0x02, 0x00,      // Vector[1]
                0x03, 0x00,      // Vector[2]
                0x04, 0x00,      // Vector[3]
                0x05, 0x00,      // Vector[4]
                0x0a, 0x31, 0x01 // Root
            },
            FLEXI_TYPE_VECTOR_UINT});

    RunTypedVector<uint16_t>(params);
}

TEST_CASE("Write Typed Vector (Uint32)", "[write_int]")
{
    using params_t = WriteIntVectorParams<uint32_t>;
    params_t params = GENERATE( //
        params_t{{1, 2},
            {
                0x01, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x08, 0x46, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        params_t{{1, 2, 3},
            {
                0x01, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, // Vector[2]
                0x0c, 0x52, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        params_t{{1, 2, 3, 4},
            {
                0x01, 0x00, 0x00, 0x00, // Vector[1]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, // Vector[3]
                0x10, 0x5e, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        params_t{{1, 2, 3, 4, 5},
            {
                0x05, 0x00, 0x00, 0x00, // Vector length
                0x01, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, // Vector[3]
                0x05, 0x00, 0x00, 0x00, // Vector[4]
                0x14, 0x32, 0x01        // Root
            },
            FLEXI_TYPE_VECTOR_UINT});

    RunTypedVector<uint32_t>(params);
}

TEST_CASE("Write Typed Vector (Uint64)", "[write_int]")
{
    using params_t = WriteIntVectorParams<uint64_t>;
    params_t params = GENERATE( //
        params_t{{1, 2},
            {
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x10, 0x47, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT2},
        params_t{{1, 2, 3},
            {
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[2]
                0x18, 0x53, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT3},
        params_t{{1, 2, 3, 4},
            {
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[3]
                0x20, 0x5f, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT4},
        params_t{{1, 2, 3, 4, 5},
            {
                0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector length
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[0]
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[1]
                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[2]
                0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[3]
                0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Vector[4]
                0x28, 0x33, 0x01                                // Root
            },
            FLEXI_TYPE_VECTOR_UINT});

    RunTypedVector<uint64_t>(params);
}
