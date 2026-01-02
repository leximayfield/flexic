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

TEST_CASE("Vector of ints", "[write_vector]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    REQUIRE(FLEXI_OK == flexi_write_bool(fwriter, NULL, true));
    REQUIRE(FLEXI_OK == flexi_write_sint(fwriter, NULL, INT16_MAX));
    REQUIRE(FLEXI_OK == flexi_write_indirect_sint(fwriter, NULL, INT32_MAX));
    REQUIRE(FLEXI_OK == flexi_write_uint(fwriter, NULL, UINT16_MAX));
    REQUIRE(FLEXI_OK == flexi_write_indirect_uint(fwriter, NULL, UINT32_MAX));
    REQUIRE(FLEXI_OK == flexi_write_vector(fwriter, NULL, 5, FLEXI_WIDTH_2B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    std::vector<uint8_t> expected = {
        0xff, 0xff, 0xff, 0x7f,       // Indirect int
        0xff, 0xff, 0xff, 0xff,       // Indirect uint
        0x05, 0x00,                   // Vector length (stride 2)
        0x01, 0x00,                   // [0] Bool
        0xff, 0x7f,                   // [1] Int
        0x0e, 0x00,                   // [2] Indirect int
        0xff, 0xff,                   // [3] Uint
        0x0e, 0x00,                   // [4] Indirect uint
        0x68, 0x05, 0x1a, 0x09, 0x1e, // Vector types
        0x0f, 0x29, 0x01              // Root offset
    };

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_VECTOR == flexi_cursor_type(&cursor));
    REQUIRE(2 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};
    bool b = false;

    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_bool(&vcursor, &b));
    REQUIRE(b == true);

    int64_t s64 = 0;

    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_sint(&vcursor, &s64));
    REQUIRE(s64 == INT16_MAX);
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 2, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_sint(&vcursor, &s64));
    REQUIRE(s64 == INT32_MAX);

    uint64_t u64 = 0;

    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 3, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_uint(&vcursor, &u64));
    REQUIRE(u64 == UINT16_MAX);
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 4, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_uint(&vcursor, &u64));
    REQUIRE(u64 == UINT32_MAX);
}

TEST_CASE("Vector of floats", "[write_vector]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    REQUIRE(FLEXI_OK == flexi_write_f32(fwriter, NULL, PI_VALUE_FLT));
    REQUIRE(FLEXI_OK == flexi_write_indirect_f32(fwriter, NULL, PI_VALUE_FLT));
    REQUIRE(FLEXI_OK == flexi_write_f64(fwriter, NULL, PI_VALUE_DBL));
    REQUIRE(FLEXI_OK == flexi_write_indirect_f64(fwriter, NULL, PI_VALUE_DBL));
    REQUIRE(FLEXI_OK == flexi_write_vector(fwriter, NULL, 4, FLEXI_WIDTH_8B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    std::vector<uint8_t> expected = {//
        // Indirect float
        0xdb, 0x0f, 0x49, 0x40,
        // Padding
        0x00, 0x00, 0x00, 0x00,
        // Indirect double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // Vector length (stride 8)
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [0] Float (widened)
        0x00, 0x00, 0x00, 0x60, 0xfb, 0x21, 0x09, 0x40,
        // [1] Indirect float
        0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [2] Double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // [2] Indirect double
        0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Vector types
        0x0e, 0x22, 0x0f, 0x23,
        // Root
        0x24, 0x2b, 0x01};

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_VECTOR == flexi_cursor_type(&cursor));
    REQUIRE(8 == flexi_cursor_width(&cursor));
    REQUIRE(4 == flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};
    float f32 = 0.0f;

    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f32(&vcursor, &f32));
    REQUIRE_THAT(f32, WithinRel(PI_VALUE_FLT));
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f32(&vcursor, &f32));
    REQUIRE_THAT(f32, WithinRel(PI_VALUE_FLT));

    double f64 = 0.0;

    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 2, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f64(&vcursor, &f64));
    REQUIRE_THAT(f64, WithinRel(PI_VALUE_DBL));
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 3, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f64(&vcursor, &f64));
    REQUIRE_THAT(f64, WithinRel(PI_VALUE_DBL));
}

TEST_CASE("Vector of strings and blobs", "[write_vector]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    static constexpr std::array<uint8_t, 8> BLOB = {
        0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};

    REQUIRE(FLEXI_OK == flexi_write_strlen(fwriter, NULL, "xyzzy"));
    REQUIRE(FLEXI_OK ==
            flexi_write_blob(fwriter, NULL, BLOB.data(), std::size(BLOB), 1));
    REQUIRE(FLEXI_OK == flexi_write_vector(fwriter, NULL, 2, FLEXI_WIDTH_1B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    std::vector<uint8_t> expected = {// String
        0x05, 'x', 'y', 'z', 'z', 'y', '\0',
        // Blob
        0x08, 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1,
        // Vector length (stride 1)
        0x02,
        // [0] String offset
        0x10,
        // [1] Blob offset
        0x0a,
        // Vector types
        0x14, 0x64,
        // Root
        0x04, 0x28, 0x01};

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_VECTOR == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(2 == flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};

    const char *str = nullptr;
    flexi_ssize_t len = -1;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    REQUIRE(5 == flexi_cursor_length(&vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_string(&vcursor, &str, &len));
    REQUIRE(5 == len);
    REQUIRE_THAT(str, Equals("xyzzy"));

    const uint8_t *blob = nullptr;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_blob(&vcursor, &blob, &len));
    REQUIRE(8 == flexi_cursor_length(&vcursor));
    REQUIRE(8 == len);
    for (size_t i = 0; i < std::size(BLOB); i++) {
        REQUIRE(blob[i] == BLOB[i]);
    }
}

TEST_CASE("Aligned Vector (4 bytes)", "[write_vector]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    static constexpr std::array<uint8_t, 8> BLOB = {
        0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};

    REQUIRE(FLEXI_OK == flexi_write_strlen(fwriter, NULL, "xyzzy"));
    REQUIRE(FLEXI_OK ==
            flexi_write_blob(fwriter, NULL, &BLOB[0], std::size(BLOB), 4));
    REQUIRE(FLEXI_OK == flexi_write_vector(fwriter, NULL, 2, FLEXI_WIDTH_1B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    std::vector<uint8_t> expected = {// String
        0x05, 'x', 'y', 'z', 'z', 'y', '\0',
        // Padding
        0x00,
        // Blob
        0x08, 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1,
        // Vector length (stride 1)
        0x02,
        // [0] String offset
        0x11,
        // [1] Blob offset
        0x0a,
        // Vector types
        0x14, 0x64,
        // Root
        0x04, 0x28, 0x01};

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_VECTOR == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(2 == flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};

    const char *str = nullptr;
    flexi_ssize_t len = -1;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_string(&vcursor, &str, &len));
    REQUIRE(5 == len);
    REQUIRE_THAT(str, Equals("xyzzy"));

    const uint8_t *blob = nullptr;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_blob(&vcursor, &blob, &len));
    REQUIRE(8 == len);
    for (size_t i = 0; i < std::size(BLOB); i++) {
        REQUIRE(blob[i] == BLOB[i]);
    }
}

TEST_CASE("Aligned Vector (16 bytes)", "[write_vector]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    static constexpr std::array<uint8_t, 8> BLOB = {
        0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};

    REQUIRE(FLEXI_OK == flexi_write_strlen(fwriter, NULL, "xyzzy"));
    REQUIRE(FLEXI_OK ==
            flexi_write_blob(fwriter, NULL, &BLOB[0], std::size(BLOB), 16));
    REQUIRE(FLEXI_OK == flexi_write_vector(fwriter, NULL, 2, FLEXI_WIDTH_1B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    std::vector<uint8_t> expected = {// String
        0x05, 'x', 'y', 'z', 'z', 'y', '\0',
        // Padding
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Blob
        0x08, 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1,
        // Vector length (stride 1)
        0x02,
        // [0] String offset
        0x19,
        // [1] Blob offset
        0x0a,
        // Vector types
        0x14, 0x64,
        // Root
        0x04, 0x28, 0x01};

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_VECTOR == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(2 == flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};

    const char *str = nullptr;
    flexi_ssize_t len = -1;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 0, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_string(&vcursor, &str, &len));
    REQUIRE(5 == len);
    REQUIRE_THAT(str, Equals("xyzzy"));

    const uint8_t *blob = nullptr;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_vector_index(&cursor, 1, &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_blob(&vcursor, &blob, &len));
    REQUIRE(8 == len);
    for (size_t i = 0; i < std::size(BLOB); i++) {
        REQUIRE(blob[i] == BLOB[i]);
    }
}

TEST_CASE("Aligned Vector (Too small param)", "[write_vector]")
{
    // The size of this string is designed to induce a situation in the
    // implementation where our first guess at the offset value is wrong.

    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    std::string str;
    str.resize(UINT16_MAX - 2, 'x');

    REQUIRE(FLEXI_OK ==
            flexi_write_string(fwriter, NULL, str.c_str(), str.length()));
    REQUIRE(FLEXI_OK == flexi_write_uint(fwriter, NULL, 128));
    REQUIRE(FLEXI_OK == flexi_write_vector(fwriter, NULL, 2, FLEXI_WIDTH_1B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_VECTOR == flexi_cursor_type(&cursor));
    REQUIRE(4 == flexi_cursor_width(&cursor));
    REQUIRE(2 == flexi_cursor_length(&cursor));
}

TEST_CASE("Aligned Vector (Vector bool)", "[write_vector]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    {
        static constexpr std::array<bool, 5> data = {
            true, false, false, true, true};

        REQUIRE(FLEXI_OK == flexi_write_typed_vector_bool(fwriter, NULL,
                                data.data(), data.size()));
        REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));
    }

    std::vector<uint8_t> expected = {
        0x05, 0x01, 0x00, 0x00, 0x01, 0x01, 0x05, 0x90, 0x01};

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_VECTOR_BOOL == flexi_cursor_type(&cursor));
    REQUIRE(1 == flexi_cursor_width(&cursor));
    REQUIRE(5 == flexi_cursor_length(&cursor));

    {
        const void *data;
        flexi_type_e type;
        int stride;
        flexi_ssize_t count;
        REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&cursor, &data,
                                &type, &stride, &count));
        REQUIRE(FLEXI_TYPE_VECTOR_BOOL == type);
        REQUIRE(1 == stride);
        REQUIRE(5 == count);

        auto boolData = static_cast<const bool *>(data);

        size_t i = 0;
        REQUIRE(true == boolData[i++]);
        REQUIRE(false == boolData[i++]);
        REQUIRE(false == boolData[i++]);
        REQUIRE(true == boolData[i++]);
        REQUIRE(true == boolData[i++]);
    }
}
