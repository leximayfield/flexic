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

#if FLEXI_FEATURE_PARSER

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
        return true;
    },
    [](const char *key, int64_t value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(sint_s{key, value});
        return true;
    },
    [](const char *key, uint64_t value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(uint_s{key, value});
        return true;
    },
    [](const char *key, float value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(f32_s{key, value});
        return true;
    },
    [](const char *key, double value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(f64_s{key, value});
        return true;
    },
    [](const char *key, const char *str, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(key_s{key, str});
        return true;
    },
    [](const char *key, const char *str, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(str_s{key, str, len});
        return true;
    },
    [](const char *key, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(mapbegin_s{key, len});
        return true;
    },
    [](void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(mapend_s{});
        return true;
    },
    [](const char *key, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(vecbegin_s{key, len});
        return true;
    },
    [](void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(vecend_s{});
        return true;
    },
    [](const char *key, const void *ptr, flexi_type_e type, int width,
        flexi_ssize_t count, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(typedvec_s{key, ptr, type, width, count, user});
        return true;
    },
    [](const char *key, const void *ptr, flexi_ssize_t len, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(blob_s{key, ptr, len});
        return true;
    },
    [](const char *key, bool value, void *user) {
        auto results = static_cast<Results *>(user);
        results->push_back(bool_s{key, value});
        return true;
    },
};

TEST_CASE("Parse basic types", "[parser]")
{
    std::string data = ReadFileToString("basic_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    Results results;
    REQUIRE(FLEXI_OK == flexi_parse_cursor(&g_parser, &cursor, &results));

    REQUIRE(13 == results.size());

    size_t i = 0;
    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(11 == value->len);
    }

    {
        auto value = std::get_if<null_s>(&results[i++]);
        REQUIRE(value);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(1 == value->value);
    }

    {
        auto value = std::get_if<uint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(2 == value->value);
    }

    {
        auto value = std::get_if<key_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("Key", Equals(value->str));
    }

    {
        auto value = std::get_if<str_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("Str", Equals(value->str));
        REQUIRE(3 == value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(3 == value->value);
    }

    {
        auto value = std::get_if<uint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(4 == value->value);
    }

    {
        auto value = std::get_if<f32_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT(PI_VALUE_FLT / 2, WithinRel(value->value));
    }

    {
        auto value = std::get_if<f64_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT(PI_VALUE_DBL, WithinRel(value->value));
    }

    {
        auto value = std::get_if<blob_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(4 == value->len);
        REQUIRE(0 == memcmp("blob", value->ptr, 4));
    }

    {
        auto value = std::get_if<bool_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(true == value->value);
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        REQUIRE(value);
    }
}

TEST_CASE("Parse nested types", "[parser]")
{
    std::string data = ReadFileToString("nested_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    Results results;
    REQUIRE(FLEXI_OK == flexi_parse_cursor(&g_parser, &cursor, &results));

    REQUIRE(14 == results.size());

    size_t i = 0;
    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(3 == value->len);
    }

    {
        auto value = std::get_if<mapbegin_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(2 == value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("bar", Equals(value->key));
        REQUIRE(2 == value->value);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("foo", Equals(value->key));
        REQUIRE(1 == value->value);
    }

    {
        auto value = std::get_if<mapend_s>(&results[i++]);
        REQUIRE(value);
    }

    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(2 == value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(4 == value->value);
    }

    {
        auto value = std::get_if<f32_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT((PI_VALUE_FLT / 2) * 3, WithinRel(value->value));
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        REQUIRE(value);
    }

    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(2 == value->len);
    }

    {
        auto value = std::get_if<sint_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(8 == value->value);
    }

    {
        auto value = std::get_if<f64_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT(PI_VALUE_DBL * 2, WithinRel(value->value));
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        REQUIRE(value);
    }

    {
        auto value = std::get_if<vecend_s>(&results[i++]);
        REQUIRE(value);
    }
}

TEST_CASE("Parse typed vectors", "[parser]")
{
    std::string data = ReadFileToString("typed_vectors.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    Results results;
    REQUIRE(FLEXI_OK == flexi_parse_cursor(&g_parser, &cursor, &results));

    REQUIRE(19 == results.size());

    size_t i = 0;
    {
        auto value = std::get_if<mapbegin_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(14 == value->len);
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("bool_vec", Equals(value->key));
        REQUIRE(2 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_BOOL == value->type);
        REQUIRE(1 == value->width);

        const bool *ptr = static_cast<const bool *>(value->ptr);
        REQUIRE(false == ptr[0]);
        REQUIRE(true == ptr[1]);
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("float_vec", Equals(value->key));
        REQUIRE(5 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_FLOAT == value->type);
        REQUIRE(4 == value->width);

        const float *ptr = static_cast<const float *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE_THAT((PI_VALUE_FLT / 2) * (i + 1.0), WithinRel(ptr[i]));
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("float_vec2", Equals(value->key));
        REQUIRE(2 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_FLOAT2 == value->type);
        REQUIRE(8 == value->width);

        const double *ptr = static_cast<const double *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE_THAT((PI_VALUE_DBL / 2) * (i + 1.0), WithinRel(ptr[i]));
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("float_vec3", Equals(value->key));
        REQUIRE(3 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_FLOAT3 == value->type);
        REQUIRE(4 == value->width);

        const float *ptr = static_cast<const float *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE_THAT((PI_VALUE_FLT / 2) * (i + 1.0), WithinRel(ptr[i]));
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("float_vec4", Equals(value->key));
        REQUIRE(4 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_FLOAT4 == value->type);
        REQUIRE(4 == value->width);

        const float *ptr = static_cast<const float *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE_THAT((PI_VALUE_FLT / 2) * (i + 1.0), WithinRel(ptr[i]));
        }
    }

    // A typed vector of keys gets emitted as a standard vector of keys.

    {
        auto value = std::get_if<vecbegin_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("keys_vec", Equals(value->key));
        REQUIRE(2 == value->len);
    }

    {
        auto value = std::get_if<key_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE(nullptr == value->key);
        REQUIRE_THAT("foo", Equals(value->str));
    }

    {
        auto value = std::get_if<key_s>(&results[i++]);
        REQUIRE(nullptr == value->key);
        REQUIRE_THAT("bar", Equals(value->str));
    }

    {
        auto end = std::get_if<vecend_s>(&results[i++]);
        REQUIRE(end);
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("sint_vec", Equals(value->key));
        REQUIRE(5 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_SINT == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("sint_vec2", Equals(value->key));
        REQUIRE(2 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_SINT2 == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("sint_vec3", Equals(value->key));
        REQUIRE(3 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_SINT3 == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("sint_vec4", Equals(value->key));
        REQUIRE(4 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_SINT4 == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("uint_vec", Equals(value->key));
        REQUIRE(5 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_UINT == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("uint_vec2", Equals(value->key));
        REQUIRE(2 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_UINT2 == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("uint_vec3", Equals(value->key));
        REQUIRE(3 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_UINT3 == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }

    {
        auto value = std::get_if<typedvec_s>(&results[i++]);
        REQUIRE(value);
        REQUIRE_THAT("uint_vec4", Equals(value->key));
        REQUIRE(4 == value->count);
        REQUIRE(FLEXI_TYPE_VECTOR_UINT4 == value->type);
        REQUIRE(1 == value->width);

        const int8_t *ptr = static_cast<const int8_t *>(value->ptr);
        for (flexi_ssize_t i = 0; i < value->count; i++) {
            REQUIRE(i + 1 == ptr[i]);
        }
    }
}

#endif // #if FLEXI_FEATURE_PARSER
