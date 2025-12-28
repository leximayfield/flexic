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

TEST(Parser, ParseBasicTypes)
{
    std::string data = ReadFileToString("basic_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));

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

TEST(Parser, ParseNestedTypes)
{
    std::string data = ReadFileToString("nested_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));

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

TEST(Parser, ParseTypedVectors)
{
    std::string data = ReadFileToString("typed_vectors.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));

    Results results;
    ASSERT_EQ(FLEXI_OK, flexi_parse_cursor(&g_parser, &cursor, &results));

    ASSERT_EQ(19, results.size());

    size_t i = 0;
    {
        auto value = std::get_if<mapbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(14, value->len);
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
        ASSERT_STREQ("float_vec", value->key);
        ASSERT_EQ(5, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT, value->type);
        ASSERT_EQ(4, value->width);

        const float *ptr = static_cast<const float *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("float_vec2", value->key);
        ASSERT_EQ(2, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT2, value->type);
        ASSERT_EQ(8, value->width);

        const double *ptr = static_cast<const double *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("float_vec3", value->key);
        ASSERT_EQ(3, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT3, value->type);
        ASSERT_EQ(4, value->width);

        const float *ptr = static_cast<const float *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("float_vec4", value->key);
        ASSERT_EQ(4, value->count);
        ASSERT_EQ(FLEXI_TYPE_VECTOR_FLOAT4, value->type);
        ASSERT_EQ(4, value->width);

        const float *ptr = static_cast<const float *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), ptr[i]);
        }
    }

    // A typed vector of keys gets emitted as a standard vector of keys.

    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_STREQ("keys_vec", value->key);
        ASSERT_EQ(2, value->len);
    }

    {
        auto value = std::get_if<key_s>(&results[i++]);
        ASSERT_NE(nullptr, value);
        ASSERT_EQ(nullptr, value->key);
        ASSERT_STREQ("foo", value->str);
    }

    {
        auto value = std::get_if<key_s>(&results[i++]);
        ASSERT_EQ(nullptr, value->key);
        ASSERT_STREQ("bar", value->str);
    }

    {
        auto end = std::get_if<vecend_s>(&results[i++]);
        ASSERT_NE(nullptr, end);
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
