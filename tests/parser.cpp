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

#include <variant>

struct null_s {
    const char *key = nullptr;
};

struct sint_s {
    const char *key = nullptr;
    int64_t value = 0;
};

struct uint_s {
    const char *key = nullptr;
    uint64_t value = 0;
};

struct f32_s {
    const char *key = nullptr;
    float value = 0.0f;
};

struct f64_s {
    const char *key = nullptr;
    double value = 0.0;
};

struct key_s {
    const char *key = nullptr;
    const char *str = "";
};

struct str_s {
    const char *key = nullptr;
    const char *str = "";
    flexi_ssize_t len = 0;
};

struct mapbegin_s {
    const char *key = nullptr;
    flexi_ssize_t len = 0;
};

struct mapend_s {};

struct vecbegin_s {
    const char *key = nullptr;
    flexi_ssize_t len = 0;
};

struct vecend_s {};

struct typedvec_s {
    const char *key = nullptr;
    const void *ptr = nullptr;
    flexi_type_e type = FLEXI_TYPE_NULL;
    int width = 0;
    flexi_ssize_t count = 0;
    void *user = nullptr;
};

struct blob_s {
    const char *key = nullptr;
    const void *ptr = nullptr;
    flexi_ssize_t len = 0;
};

struct bool_s {
    const char *key = nullptr;
    bool value = false;
};

using result_t =
    std::variant<null_s, sint_s, uint_s, f32_s, f64_s, key_s, str_s, mapbegin_s,
        mapend_s, vecbegin_s, vecend_s, typedvec_s, blob_s, bool_s>;

using Results = std::vector<result_t>;

static constexpr flexi_parser_s g_parser{
    [](const char *key, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(null_s{key});
    },
    [](const char *key, int64_t value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(sint_s{key, value});
    },
    [](const char *key, uint64_t value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(uint_s{key, value});
    },
    [](const char *key, float value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(f32_s{key, value});
    },
    [](const char *key, double value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(f64_s{key, value});
    },
    [](const char *key, const char *str, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(key_s{key, str});
    },
    [](const char *key, const char *str, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(str_s{key, str, len});
    },
    [](const char *key, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(mapbegin_s{key, len});
    },
    [](void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(mapend_s{});
    },
    [](const char *key, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(vecbegin_s{key, len});
    },
    [](void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(vecend_s{});
    },
    [](const char *key, const void *ptr, flexi_type_e type, int width,
        flexi_ssize_t count, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(typedvec_s{key, ptr, type, width, count, user});
    },
    [](const char *key, const void *ptr, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(blob_s{key, ptr, len});
    },
    [](const char *key, bool value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(bool_s{key, value});
    },
};

constexpr std::array<uint8_t, 55> g_basic_types = {0x4B, 0x65, 0x79, 0x00, 0x03,
    0x53, 0x74, 0x72, 0x00, 0x03, 0x04, 0x00, 0xDB, 0x0F, 0xC9, 0x3F, 0x18,
    0x2D, 0x44, 0x54, 0xFB, 0x21, 0x09, 0x40, 0x04, 0x62, 0x6C, 0x6F, 0x62,
    0x0B, 0x00, 0x01, 0x02, 0x21, 0x1D, 0x1A, 0x1A, 0x19, 0x16, 0x0E, 0x01,
    0x00, 0x04, 0x08, 0x10, 0x14, 0x18, 0x1C, 0x22, 0x23, 0x64, 0x68, 0x16,
    0x28, 0x01};

TEST(Parser, ParseBasicTypes)
{
    flexi_buffer_s buffer =
        flexi_make_buffer(&g_basic_types[0], std::size(g_basic_types));

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));

    Results results;
    ASSERT_EQ(FLEXI_OK, flexi_parse_cursor(&g_parser, &cursor, &results));

    ASSERT_EQ(13, results.size());

    size_t i = 0;
    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(11, value->len);
    }

    {
        auto value = std::get_if<null_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(1, value->value);
    }

    {
        auto value = std::get_if<uint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(2, value->value);
    }

    {
        auto value = std::get_if<key_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("Key", value->str);
    }

    {
        auto value = std::get_if<str_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("Str", value->str);
        ASSERT_EQ(3, value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(3, value->value);
    }

    {
        auto value = std::get_if<uint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(4, value->value);
    }

    {
        auto value = std::get_if<f32_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_FLOAT_EQ(PI_VALUE / 2, value->value);
    }

    {
        auto value = std::get_if<f64_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(PI_VALUE, value->value);
    }

    {
        auto value = std::get_if<blob_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(4, value->len);
        ASSERT_EQ(0, memcmp("blob", value->ptr, 4));
    }

    {
        auto value = std::get_if<bool_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(true, value->value);
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
    }
}

constexpr std::array<uint8_t, 76> g_nested_types = {0x66, 0x6F, 0x6F, 0x00,
    0x62, 0x61, 0x72, 0x00, 0x02, 0x05, 0x0A, 0x02, 0x01, 0x02, 0x02, 0x01,
    0x04, 0x04, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0xE4, 0xCB, 0x96, 0x40, 0x06, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x18, 0x2D, 0x44, 0x54, 0xFB, 0x21, 0x19, 0x40,
    0x07, 0x0F, 0x03, 0x35, 0x2C, 0x15, 0x24, 0x2A, 0x2B, 0x06, 0x28, 0x01};

TEST(Parser, ParseNestedTypes)
{
    flexi_buffer_s buffer =
        flexi_make_buffer(&g_nested_types[0], std::size(g_nested_types));

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));

    Results results;
    ASSERT_EQ(FLEXI_OK, flexi_parse_cursor(&g_parser, &cursor, &results));

    ASSERT_EQ(14, results.size());

    size_t i = 0;
    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(3, value->len);
    }

    {
        auto value = std::get_if<mapbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(2, value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("bar", value->key);
        ASSERT_EQ(2, value->value);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("foo", value->key);
        ASSERT_EQ(1, value->value);
    }

    {
        auto value = std::get_if<mapend_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
    }

    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(2, value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(4, value->value);
    }

    {
        auto value = std::get_if<f32_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_FLOAT_EQ((PI_VALUE / 2) * 3, value->value);
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
    }

    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(2, value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(8, value->value);
    }

    {
        auto value = std::get_if<f64_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(PI_VALUE * 2, value->value);
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
    }
}

constexpr std::array<uint8_t, 154> g_typed_vectors = {0x73, 0x69, 0x6E, 0x74,
    0x5F, 0x76, 0x65, 0x63, 0x32, 0x00, 0x01, 0x02, 0x73, 0x69, 0x6E, 0x74,
    0x5F, 0x76, 0x65, 0x63, 0x33, 0x00, 0x01, 0x02, 0x03, 0x73, 0x69, 0x6E,
    0x74, 0x5F, 0x76, 0x65, 0x63, 0x34, 0x00, 0x01, 0x02, 0x03, 0x04, 0x73,
    0x69, 0x6E, 0x74, 0x5F, 0x76, 0x65, 0x63, 0x00, 0x05, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x75, 0x69, 0x6E, 0x74, 0x5F, 0x76, 0x65, 0x63, 0x32, 0x00,
    0x01, 0x02, 0x75, 0x69, 0x6E, 0x74, 0x5F, 0x76, 0x65, 0x63, 0x33, 0x00,
    0x01, 0x02, 0x03, 0x75, 0x69, 0x6E, 0x74, 0x5F, 0x76, 0x65, 0x63, 0x34,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x75, 0x69, 0x6E, 0x74, 0x5F, 0x76, 0x65,
    0x63, 0x00, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05, 0x62, 0x6F, 0x6F, 0x6C,
    0x5F, 0x76, 0x65, 0x63, 0x00, 0x02, 0x00, 0x01, 0x09, 0x0D, 0x53, 0x7B,
    0x70, 0x64, 0x21, 0x49, 0x3E, 0x32, 0x09, 0x01, 0x09, 0x0F, 0x55, 0x7D,
    0x72, 0x66, 0x23, 0x4B, 0x40, 0x34, 0x90, 0x2C, 0x40, 0x4C, 0x58, 0x30,
    0x44, 0x50, 0x5C, 0x12, 0x24, 0x01};

TEST(Parser, ParseTypedVectors)
{
    flexi_buffer_s buffer =
        flexi_make_buffer(&g_typed_vectors[0], std::size(g_typed_vectors));

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_buffer(&buffer, &cursor));

    Results results;
    ASSERT_EQ(FLEXI_OK, flexi_parse_cursor(&g_parser, &cursor, &results));

    ASSERT_EQ(11, results.size());

    size_t i = 0;
    {
        auto value = std::get_if<mapbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(9, value->len);
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("bool_vec", value->key);
        ASSERT_EQ(2, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_BOOL, value->type);
        ASSERT_EQ(1, value->width);

        const bool *ptr = static_cast<const bool *>(value->ptr);
        ASSERT_EQ(false, ptr[0]);
        ASSERT_EQ(true, ptr[1]);
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("sint_vec", value->key);
        ASSERT_EQ(5, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_SINT, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("sint_vec2", value->key);
        ASSERT_EQ(2, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_SINT2, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("sint_vec3", value->key);
        ASSERT_EQ(3, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_SINT3, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("sint_vec4", value->key);
        ASSERT_EQ(4, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_SINT4, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("uint_vec", value->key);
        ASSERT_EQ(5, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_UINT, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("uint_vec2", value->key);
        ASSERT_EQ(2, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_UINT2, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("uint_vec3", value->key);
        ASSERT_EQ(3, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_UINT3, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("uint_vec4", value->key);
        ASSERT_EQ(4, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_UINT4, value->type);
        ASSERT_EQ(1, value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_EQ(i + 1, ptr[i]);
        }
    }
}
