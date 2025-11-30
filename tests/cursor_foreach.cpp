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

static constexpr std::array<uint8_t, 28> g_test_vector = {
    0xff, 0xff, 0xff, 0x7f,       // Indirect int
    0xff, 0xff, 0xff, 0xff,       // Indirect uint
    0x05, 0x00,                   // Vector length (stride 2)
    0x01, 0x00,                   // [0] Bool
    0xff, 0x7f,                   // [1] Int
    0x0e, 0x00,                   // [2] Indirect int
    0xff, 0xff,                   // [3] Uint
    0x0e, 0x00,                   // [4] Indirect uint
    0x68, 0x05, 0x1a, 0x09, 0x1e, // Vector types
    0x0f, 0x29, 0x01,             // Root offset
};

static constexpr std::array<uint8_t, 84> g_test_map = {
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
    0x0f, 0x25, 0x01,             // Root
};

struct foreach_result_s {
    flexi_cursor_s cursor;
    const char *key;
};

using foreach_results_t = std::vector<foreach_result_s>;

static bool
ForeachCB(const char *key, flexi_cursor_s *cursor, void *user)
{
    auto results = static_cast<foreach_results_t *>(user);
    results->push_back(foreach_result_s{*cursor, key});
    return true;
}

TEST(CursorForeach, ForeachVector)
{
    flexi_buffer_s buffer =
        flexi_make_buffer(g_test_vector.data(), g_test_vector.size());

    flexi_cursor_s cursor;
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));

    foreach_results_t results;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_foreach(&cursor, &ForeachCB, &results));

    ASSERT_EQ(5, results.size());

    size_t i = 0;
    {
        ASSERT_EQ(nullptr, results[i].key);
        ASSERT_EQ(FLEXI_TYPE_BOOL, flexi_cursor_type(&results[i].cursor));

        bool val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&results[i].cursor, &val));
        ASSERT_EQ(true, val);
    }

    i++;
    {
        ASSERT_EQ(nullptr, results[i].key);
        ASSERT_EQ(FLEXI_TYPE_SINT, flexi_cursor_type(&results[i].cursor));

        int64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&results[i].cursor, &val));
        ASSERT_EQ(INT16_MAX, val);
    }

    i++;
    {
        ASSERT_EQ(nullptr, results[i].key);
        ASSERT_EQ(FLEXI_TYPE_INDIRECT_SINT,
            flexi_cursor_type(&results[i].cursor));

        int64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&results[i].cursor, &val));
        ASSERT_EQ(INT32_MAX, val);
    }

    i++;
    {
        ASSERT_EQ(nullptr, results[i].key);
        ASSERT_EQ(FLEXI_TYPE_UINT, flexi_cursor_type(&results[i].cursor));

        uint64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&results[i].cursor, &val));
        ASSERT_EQ(UINT16_MAX, val);
    }

    i++;
    {
        ASSERT_EQ(nullptr, results[i].key);
        ASSERT_EQ(FLEXI_TYPE_INDIRECT_UINT,
            flexi_cursor_type(&results[i].cursor));

        uint64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&results[i].cursor, &val));
        ASSERT_EQ(UINT32_MAX, val);
    }
}

TEST(CursorForeach, ForeachMap)
{
    flexi_buffer_s buffer =
        flexi_make_buffer(g_test_map.data(), g_test_map.size());

    flexi_cursor_s cursor;
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));

    foreach_results_t results;
    ASSERT_EQ(FLEXI_OK, flexi_cursor_foreach(&cursor, &ForeachCB, &results));

    ASSERT_EQ(5, results.size());

    size_t i = 0;
    {
        ASSERT_STREQ("bool", results[i].key);
        ASSERT_EQ(FLEXI_TYPE_BOOL, flexi_cursor_type(&results[i].cursor));

        bool val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_bool(&results[i].cursor, &val));
        ASSERT_EQ(true, val);
    }

    i++;
    {
        ASSERT_STREQ("indirect_sint", results[i].key);
        ASSERT_EQ(FLEXI_TYPE_INDIRECT_SINT,
            flexi_cursor_type(&results[i].cursor));

        int64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&results[i].cursor, &val));
        ASSERT_EQ(INT32_MAX, val);
    }

    i++;
    {
        ASSERT_STREQ("indirect_uint", results[i].key);
        ASSERT_EQ(FLEXI_TYPE_INDIRECT_UINT,
            flexi_cursor_type(&results[i].cursor));

        uint64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&results[i].cursor, &val));
        ASSERT_EQ(UINT32_MAX, val);
    }

    i++;
    {
        ASSERT_STREQ("sint", results[i].key);
        ASSERT_EQ(FLEXI_TYPE_SINT, flexi_cursor_type(&results[i].cursor));

        int64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_sint(&results[i].cursor, &val));
        ASSERT_EQ(INT16_MAX, val);
    }

    i++;
    {
        ASSERT_STREQ("uint", results[i].key);
        ASSERT_EQ(FLEXI_TYPE_UINT, flexi_cursor_type(&results[i].cursor));

        uint64_t val;
        ASSERT_EQ(FLEXI_OK, flexi_cursor_uint(&results[i].cursor, &val));
        ASSERT_EQ(UINT16_MAX, val);
    }
}
