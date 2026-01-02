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

TEST_CASE("Immediate", "[write_map]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    REQUIRE(FLEXI_OK == flexi_write_uint(fwriter, "uint", UINT16_MAX));
    REQUIRE(FLEXI_OK == flexi_write_sint(fwriter, "sint", INT16_MAX));
    REQUIRE(FLEXI_OK == flexi_write_bool(fwriter, "bool", true));
    REQUIRE(FLEXI_OK == flexi_write_map(fwriter, NULL, 3, FLEXI_WIDTH_2B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    // There should be nothing on the stack, especially not the keys that
    // flexi_write_map used internally.
    flexi_value_s *stack_value;
    REQUIRE(FLEXI_ERR_BADSTACK ==
            flexi_writer_debug_stack_at(fwriter, 0, &stack_value));
    REQUIRE(0 == flexi_writer_debug_stack_count(fwriter));

    std::vector<uint8_t> expected = {
        'u', 'i', 'n', 't', '\0', // Key values
        's', 'i', 'n', 't', '\0', //
        'b', 'o', 'o', 'l', '\0', //
        0x03,                     // Map keys vector length
        0x06,                     // Keys[0] "bool"
        0x0c,                     // Keys[1] "sint"
        0x12,                     // Keys[2] "uint"
        0x00,                     // Padding
        0x04, 0x00,               // Keys vector offset
        0x01, 0x00,               // Keys vector stride
        0x03, 0x00,               // Map values vector length
        0x01, 0x00,               // Values[0] Bool
        0xff, 0x7f,               // Values[1] Int
        0xff, 0xff,               // Values[2] Uint
        0x68, 0x05, 0x09,         // Types
        0x09, 0x25, 0x01          // Root
    };
    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_MAP == flexi_cursor_type(&cursor));
    REQUIRE(2 == flexi_cursor_width(&cursor));

    flexi_cursor_s value{};

    bool vbool = false;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "bool", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_bool(&value, &vbool));
    REQUIRE(true == vbool);

    int64_t vsint = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "sint", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_sint(&value, &vsint));
    REQUIRE(INT16_MAX == vsint);

    uint64_t vuint = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "uint", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_uint(&value, &vuint));
    REQUIRE(UINT16_MAX == vuint);
}

TEST_CASE("Map of Ints", "[write_map]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "bool"));
    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "sint"));
    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "indirect_sint"));
    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "uint"));
    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "indirect_uint"));
    flexi_stack_idx_t keyset = -1;
    REQUIRE(FLEXI_OK ==
            flexi_write_map_keys(fwriter, 5, FLEXI_WIDTH_1B, &keyset));
    REQUIRE(0 == keyset);

    REQUIRE(FLEXI_OK == flexi_write_bool(fwriter, "bool", true));
    REQUIRE(FLEXI_OK == flexi_write_sint(fwriter, "sint", INT16_MAX));
    REQUIRE(FLEXI_OK ==
            flexi_write_indirect_sint(fwriter, "indirect_sint", INT32_MAX));
    REQUIRE(FLEXI_OK == flexi_write_uint(fwriter, "uint", UINT16_MAX));
    REQUIRE(FLEXI_OK ==
            flexi_write_indirect_uint(fwriter, "indirect_uint", UINT32_MAX));
    REQUIRE(FLEXI_OK ==
            flexi_write_map_values(fwriter, NULL, keyset, 5, FLEXI_WIDTH_2B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    std::vector<uint8_t> expected = {
        'b', 'o', 'o', 'l', '\0', // Key values
        's', 'i', 'n', 't', '\0', //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 's', 'i', 'n', 't',
        '\0',                     //
        'u', 'i', 'n', 't', '\0', //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 'u', 'i', 'n', 't',
        '\0',                         //
        0x05,                         // Map keys vector length
        0x2c,                         // Keys[0] "bool"
        0x23,                         // Keys[1] "indirect_sint"
        0x11,                         // Keys[2] "indirect_uint"
        0x2a,                         // Keys[3] "sint"
        0x18,                         // Keys[4] "uint"
        0x00, 0x00, 0x00,             // Padding
        0xff, 0xff, 0xff, 0x7f,       // Indirect int
        0xff, 0xff, 0xff, 0xff,       // Indirect uint
        0x10, 0x00,                   // Keys vector offset
        0x01, 0x00,                   // Keys vector stride
        0x05, 0x00,                   // Map values vector length
        0x01, 0x00,                   // Values[0] Bool
        0x10, 0x00,                   // Values[1] Indirect int
        0x0e, 0x00,                   // Values[2] Indirect uint
        0xff, 0x7f,                   // Values[3] Int
        0xff, 0xff,                   // Values[4] Uint
        0x68, 0x1a, 0x1e, 0x05, 0x09, // Types
        0x0f, 0x25, 0x01              // Root
    };

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_MAP == flexi_cursor_type(&cursor));
    REQUIRE(2 == flexi_cursor_width(&cursor));

    flexi_cursor_s value{};

    bool vbool = false;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "bool", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_bool(&value, &vbool));
    REQUIRE(true == vbool);

    int64_t vsint = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "sint", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_sint(&value, &vsint));
    REQUIRE(INT16_MAX == vsint);
    REQUIRE(FLEXI_OK ==
            flexi_cursor_seek_map_key(&cursor, "indirect_sint", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_sint(&value, &vsint));
    REQUIRE(INT32_MAX == vsint);

    uint64_t vuint = 0;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "uint", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_uint(&value, &vuint));
    REQUIRE(UINT16_MAX == vuint);
    REQUIRE(FLEXI_OK ==
            flexi_cursor_seek_map_key(&cursor, "indirect_uint", &value));
    REQUIRE(FLEXI_OK == flexi_cursor_uint(&value, &vuint));
    REQUIRE(UINT32_MAX == vuint);
}

TEST_CASE("Map of Floats", "[write_map]")
{
    TestWriter writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "f32"));
    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "indirect_f32"));
    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "f64"));
    REQUIRE(FLEXI_OK == flexi_write_key(fwriter, "indirect_f64"));
    flexi_stack_idx_t keyset = -1;
    REQUIRE(FLEXI_OK ==
            flexi_write_map_keys(fwriter, 4, FLEXI_WIDTH_1B, &keyset));
    REQUIRE(0 == keyset);

    REQUIRE(FLEXI_OK == flexi_write_f32(fwriter, "f32", PI_VALUE_FLT));
    REQUIRE(FLEXI_OK ==
            flexi_write_indirect_f32(fwriter, "indirect_f32", PI_VALUE_FLT));
    REQUIRE(FLEXI_OK == flexi_write_f64(fwriter, "f64", PI_VALUE_DBL));
    REQUIRE(FLEXI_OK ==
            flexi_write_indirect_f64(fwriter, "indirect_f64", PI_VALUE_DBL));
    REQUIRE(FLEXI_OK ==
            flexi_write_map_values(fwriter, NULL, keyset, 4, FLEXI_WIDTH_8B));
    REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));

    std::vector<uint8_t> expected = {'f', '3', '2', '\0',                 //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 'f', '3', '2', '\0', //
        'f', '6', '4', '\0',                                              //
        'i', 'n', 'd', 'i', 'r', 'e', 'c', 't', '_', 'f', '6', '4', '\0', //
        // Map keys vector length
        0x04,
        // Keys[0] "f32"
        0x23,
        // Keys[1] "f64"
        0x13,
        // Keys[2] "indirect_f32"
        0x21,
        // Keys[3] "indirect_f64"
        0x11,
        // Padding
        0x00,
        // Indirect float
        0xdb, 0x0f, 0x49, 0x40,
        // Padding
        0x00, 0x00, 0x00, 0x00,
        // Indirect double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // Keys vector offset (stride 8)
        0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Keys vector stride (stride 8)
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Vector length (stride 8)
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [0] Float (widened)
        0x00, 0x00, 0x00, 0x60, 0xfb, 0x21, 0x09, 0x40,
        // [1] Double
        0x18, 0x2d, 0x44, 0x54, 0xfb, 0x21, 0x09, 0x40,
        // [2] Indirect float
        0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // [3] Indirect double
        0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Vector types
        0x0e, 0x0f, 0x22, 0x23,
        // Root
        0x24, 0x27, 0x01};

    writer.AssertData(expected);

    flexi_cursor_s cursor{};
    writer.GetCursor(&cursor);

    REQUIRE(FLEXI_TYPE_MAP == flexi_cursor_type(&cursor));
    REQUIRE(8 == flexi_cursor_width(&cursor));
    REQUIRE(4 == flexi_cursor_length(&cursor));

    flexi_cursor_s vcursor{};
    float f32 = 0.0f;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "f32", &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f32(&vcursor, &f32));
    REQUIRE_THAT(f32, WithinRel(PI_VALUE_FLT));

    f32 = 0.0f;
    REQUIRE(FLEXI_OK ==
            flexi_cursor_seek_map_key(&cursor, "indirect_f32", &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f32(&vcursor, &f32));
    REQUIRE_THAT(f32, WithinRel(PI_VALUE_FLT));

    double f64 = 0.0;
    REQUIRE(FLEXI_OK == flexi_cursor_seek_map_key(&cursor, "f64", &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f64(&vcursor, &f64));
    REQUIRE_THAT(f64, WithinRel(PI_VALUE_DBL));

    f64 = 0.0;
    REQUIRE(FLEXI_OK ==
            flexi_cursor_seek_map_key(&cursor, "indirect_f64", &vcursor));
    REQUIRE(FLEXI_OK == flexi_cursor_f64(&vcursor, &f64));
    REQUIRE_THAT(f64, WithinRel(PI_VALUE_DBL));
}

TEST_CASE("Create Large Document #2", "[write_map]")
{
    TestWriterStrdup writer;
    flexi_writer_s *fwriter = writer.GetWriter();

    static std::array<float, 3> s_data = {
        (PI_VALUE_FLT / 2), PI_VALUE_FLT, (PI_VALUE_FLT / 2) * 3};
    char keybuf[16];
    char mapbuf[16];

    {
        for (int i = 0; i < 100; i++) {
            snprintf(keybuf, 16, "key-%d", i);
            REQUIRE(FLEXI_OK == flexi_write_key(fwriter, keybuf));
        }

        flexi_stack_idx_t keys_idx;
        REQUIRE(FLEXI_OK ==
                flexi_write_map_keys(fwriter, 100, FLEXI_WIDTH_4B, &keys_idx));

        for (int i = 0; i < 100; i++) {
            for (int j = 0; j < 100; j++) {
                snprintf(keybuf, 16, "key-%d", j);
                REQUIRE(FLEXI_OK == flexi_write_typed_vector_flt(fwriter,
                                        keybuf, s_data.data(), FLEXI_WIDTH_4B,
                                        s_data.size()));
            }

            snprintf(mapbuf, 16, "map-%d", i);
            REQUIRE(FLEXI_OK == flexi_write_map_values(fwriter, mapbuf,
                                    keys_idx, 100, FLEXI_WIDTH_1B));
        }

        REQUIRE(FLEXI_OK ==
                flexi_write_map(fwriter, NULL, 100, FLEXI_WIDTH_1B));
        REQUIRE(FLEXI_OK == flexi_write_finalize(fwriter));
    }

    {
        flexi_cursor_s cursor;
        writer.GetCursor(&cursor);

        for (int i = 0; i < 100; i++) {
            CAPTURE(i);
            flexi_cursor_s map;
            snprintf(mapbuf, 16, "map-%d", i);

            CAPTURE(mapbuf);
            REQUIRE(FLEXI_OK ==
                    flexi_cursor_seek_map_key(&cursor, mapbuf, &map));

            for (int j = 0; j < 100; j++) {
                flexi_cursor_s value;
                snprintf(keybuf, 16, "key-%d", i);
                REQUIRE(FLEXI_OK ==
                        flexi_cursor_seek_map_key(&map, keybuf, &value));

                REQUIRE(FLEXI_TYPE_VECTOR_FLOAT3 == flexi_cursor_type(&value));
                REQUIRE(3 == flexi_cursor_length(&value));
                REQUIRE(4 == flexi_cursor_width(&value));

                const void *data = nullptr;
                REQUIRE(FLEXI_OK == flexi_cursor_typed_vector_data(&value,
                                        &data, NULL, NULL, NULL));
                auto vec3 = static_cast<const float *>(data);
                REQUIRE_THAT(s_data[0], WithinRel(vec3[0]));
                REQUIRE_THAT(s_data[1], WithinRel(vec3[1]));
                REQUIRE_THAT(s_data[2], WithinRel(vec3[2]));
            }
        }
    }
}
