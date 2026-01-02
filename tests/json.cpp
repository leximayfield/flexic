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

#include "flexic.h"

#if FLEXI_FEATURE_JSON

#include "tests.hpp"

#include <nlohmann/json.hpp>

/******************************************************************************/

TEST_CASE("Basic Types", "[json]")
{
    std::string data = ReadFileToString("basic_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    std::string json_str;
    REQUIRE(FLEXI_OK == flexi_json_string_from_cursor(&cursor, json_str));

    nlohmann::json json;
    REQUIRE_NOTHROW(json = nlohmann::json::parse(json_str));

    REQUIRE(11 == json.size());

    size_t i = 0;
    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_null());
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_number_integer());
        REQUIRE(value == 1);
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_number_integer());
        REQUIRE(value == 2);
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_string());
        REQUIRE(value == "Key");
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_string());
        REQUIRE(value == "Str");
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_number_integer());
        REQUIRE(value == 3);
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_number_integer());
        REQUIRE(value == 4);
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_number_float());
        REQUIRE_THAT(PI_VALUE_FLT / 2, WithinRel(value.get<float>()));
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_number_float());
        REQUIRE_THAT(PI_VALUE_DBL, WithinRel(value.get<double>()));
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_string());
        std::string str{value};

        uint8_t data[8];
        flexi_ssize_t written = sizeof(data);
        REQUIRE(FLEXI_OK == flexi_json_decode_blob(str.c_str(), str.length(),
                                data, &written));

        REQUIRE(4 == written);
        REQUIRE(0 == memcmp("blob", data, 4));
    }

    {
        nlohmann::json value = json[i++];
        REQUIRE(value.is_boolean());
        REQUIRE(true == value);
    }
}

TEST_CASE("Nested Types", "[json]")
{
    std::string data = ReadFileToString("nested_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    std::string json_str;
    REQUIRE(FLEXI_OK == flexi_json_string_from_cursor(&cursor, json_str));

    nlohmann::json json;
    REQUIRE_NOTHROW(json = nlohmann::json::parse(json_str));

    REQUIRE(3 == json.size());

    size_t i = 0;
    {
        nlohmann::json object = json[i++];
        REQUIRE(object.is_object());
    }

    {
        nlohmann::json array = json[i++];
        REQUIRE(array.is_array());
        REQUIRE(2 == array.size());

        size_t j = 0;
        {
            nlohmann::json value = array[j++];
            REQUIRE(value.is_number_integer());
            REQUIRE(4 == value);
        }

        {
            nlohmann::json value = array[j++];
            REQUIRE(value.is_number_float());
            REQUIRE_THAT((PI_VALUE_FLT / 2) * 3, WithinRel(value.get<float>()));
        }
    }

    {
        nlohmann::json array = json[i++];
        REQUIRE(array.is_array());
        REQUIRE(2 == array.size());

        size_t j = 0;
        {
            nlohmann::json value = array[j++];
            REQUIRE(value.is_number_integer());
            REQUIRE(8 == value);
        }

        {
            nlohmann::json value = array[j++];
            REQUIRE(value.is_number_float());
            REQUIRE_THAT(PI_VALUE_FLT * 2, WithinRel(value.get<float>()));
        }
    }
}

TEST_CASE("Typed Vectors", "[json]")
{
    std::string data = ReadFileToString("typed_vectors.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    REQUIRE(FLEXI_OK == flexi_open_span(&span, &cursor));

    std::string json_str;
    REQUIRE(FLEXI_OK == flexi_json_string_from_cursor(&cursor, json_str));

    nlohmann::json json;
    REQUIRE_NOTHROW(json = nlohmann::json::parse(json_str));

    {
        nlohmann::json array = json["bool_vec"];
        REQUIRE(array.is_array());
        REQUIRE(2 == array.size());

        size_t i = 0;
        {
            nlohmann::json value = array[i++];
            REQUIRE(value.is_boolean());
            REQUIRE(false == value);
        }

        {
            nlohmann::json value = array[i++];
            REQUIRE(value.is_boolean());
            REQUIRE(true == value);
        }
    }

    {
        nlohmann::json array = json["float_vec2"];
        REQUIRE(array.is_array());
        REQUIRE(2 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_float());
            REQUIRE_THAT((PI_VALUE_FLT / 2) * (i + 1.0),
                WithinRel(array[i].get<float>()));
        }
    }

    {
        nlohmann::json array = json["float_vec3"];
        REQUIRE(array.is_array());
        REQUIRE(3 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            CAPTURE(i);
            REQUIRE(array[i].is_number_float());
            REQUIRE_THAT((PI_VALUE_FLT / 2) * (i + 1.0),
                WithinRel(array[i].get<float>()));
        }
    }

    {
        nlohmann::json array = json["float_vec4"];
        REQUIRE(array.is_array());
        REQUIRE(4 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_float());
            REQUIRE_THAT((PI_VALUE_FLT / 2) * (i + 1.0),
                WithinRel(array[i].get<float>()));
        }
    }

    {
        nlohmann::json array = json["float_vec"];
        REQUIRE(array.is_array());
        REQUIRE(5 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_float());
            REQUIRE_THAT((PI_VALUE_FLT / 2) * (i + 1.0),
                WithinRel(array[i].get<float>()));
        }
    }

    {
        nlohmann::json array = json["keys_vec"];
        REQUIRE(array.is_array());
        REQUIRE(2 == array.size());

        size_t i = 0;
        {
            nlohmann::json value = array[i++];
            REQUIRE(value.is_string());
            REQUIRE_THAT("foo", Equals(value));
        }

        {
            nlohmann::json value = array[i++];
            REQUIRE(value.is_string());
            REQUIRE_THAT("bar", Equals(value));
        }
    }

    {
        nlohmann::json array = json["sint_vec2"];
        REQUIRE(array.is_array());
        REQUIRE(2 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }

    {
        nlohmann::json array = json["sint_vec3"];
        REQUIRE(array.is_array());
        REQUIRE(3 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }

    {
        nlohmann::json array = json["sint_vec4"];
        REQUIRE(array.is_array());
        REQUIRE(4 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }

    {
        nlohmann::json array = json["sint_vec"];
        REQUIRE(array.is_array());
        REQUIRE(5 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec2"];
        REQUIRE(array.is_array());
        REQUIRE(2 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec3"];
        REQUIRE(array.is_array());
        REQUIRE(3 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec4"];
        REQUIRE(array.is_array());
        REQUIRE(4 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec"];
        REQUIRE(array.is_array());
        REQUIRE(5 == array.size());

        for (size_t i = 0; i < array.size(); i++) {
            REQUIRE(array[i].is_number_integer());
            REQUIRE(i + 1 == array[i]);
        }
    }
}

#endif // #if FLEXI_FEATURE_JSON
