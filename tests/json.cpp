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

TEST(CursorToJSON, BasicTypes)
{
    std::string data = ReadFileToString("basic_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));

    std::string json_str;
    ASSERT_EQ(FLEXI_OK, flexi_json_string_from_cursor(&cursor, json_str));

    nlohmann::json json;
    ASSERT_NO_THROW(json = nlohmann::json::parse(json_str));

    ASSERT_EQ(11, json.size());

    size_t i = 0;
    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_null());
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_number_integer());
        ASSERT_TRUE(value == 1);
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_number_integer());
        ASSERT_TRUE(value == 2);
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_string());
        ASSERT_TRUE(value == "Key");
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_string());
        ASSERT_TRUE(value == "Str");
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_number_integer());
        ASSERT_TRUE(value == 3);
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_number_integer());
        ASSERT_TRUE(value == 4);
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_number_float());
        ASSERT_FLOAT_EQ(PI_VALUE / 2, value);
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_number_float());
        ASSERT_EQ(PI_VALUE, value);
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_string());
        std::string str{value};

        uint8_t data[8];
        flexi_ssize_t written = sizeof(data);
        ASSERT_EQ(FLEXI_OK,
            flexi_json_decode_blob(str.c_str(), str.length(), data, &written));

        ASSERT_EQ(4, written);
        ASSERT_EQ(0, memcmp("blob", data, 4));
    }

    {
        nlohmann::json value = json[i++];
        ASSERT_TRUE(value.is_boolean());
        ASSERT_EQ(true, value);
    }
}

TEST(CursorToJSON, NestedTypes)
{
    std::string data = ReadFileToString("nested_types.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));

    std::string json_str;
    ASSERT_EQ(FLEXI_OK, flexi_json_string_from_cursor(&cursor, json_str));

    nlohmann::json json;
    ASSERT_NO_THROW(json = nlohmann::json::parse(json_str));

    ASSERT_EQ(3, json.size());

    size_t i = 0;
    {
        nlohmann::json object = json[i++];
        ASSERT_TRUE(object.is_object());
    }

    {
        nlohmann::json array = json[i++];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(2, array.size());

        size_t j = 0;
        {
            nlohmann::json value = array[j++];
            ASSERT_TRUE(value.is_number_integer());
            ASSERT_EQ(4, value);
        }

        {
            nlohmann::json value = array[j++];
            ASSERT_TRUE(value.is_number_float());
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * 3, value);
        }
    }

    {
        nlohmann::json array = json[i++];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(2, array.size());

        size_t j = 0;
        {
            nlohmann::json value = array[j++];
            ASSERT_TRUE(value.is_number_integer());
            ASSERT_EQ(8, value);
        }

        {
            nlohmann::json value = array[j++];
            ASSERT_TRUE(value.is_number_float());
            ASSERT_FLOAT_EQ(PI_VALUE * 2, value);
        }
    }
}

TEST(CursorToJSON, TypedVectors)
{
    std::string data = ReadFileToString("typed_vectors.flexbuf");
    flexi_span_s span = flexi_make_span(data.data(), data.size());

    flexi_cursor_s cursor{};
    ASSERT_EQ(FLEXI_OK, flexi_open_span(&span, &cursor));

    std::string json_str;
    ASSERT_EQ(FLEXI_OK, flexi_json_string_from_cursor(&cursor, json_str));

    nlohmann::json json;
    ASSERT_NO_THROW(json = nlohmann::json::parse(json_str));

    {
        nlohmann::json array = json["bool_vec"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(2, array.size());

        size_t i = 0;
        {
            nlohmann::json value = array[i++];
            ASSERT_TRUE(value.is_boolean());
            ASSERT_EQ(false, value);
        }

        {
            nlohmann::json value = array[i++];
            ASSERT_TRUE(value.is_boolean());
            ASSERT_EQ(true, value);
        }
    }

    {
        nlohmann::json array = json["float_vec2"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(2, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_float());
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), array[i]);
        }
    }

    {
        nlohmann::json array = json["float_vec3"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(3, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_float());
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), array[i]);
        }
    }

    {
        nlohmann::json array = json["float_vec4"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(4, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_float());
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), array[i]);
        }
    }

    {
        nlohmann::json array = json["float_vec"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(5, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_float());
            ASSERT_FLOAT_EQ((PI_VALUE / 2) * (i + 1.0), array[i]);
        }
    }

    {
        nlohmann::json array = json["keys_vec"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(2, array.size());

        size_t i = 0;
        {
            nlohmann::json value = array[i++];
            ASSERT_TRUE(value.is_string());
            ASSERT_EQ("foo", value);
        }

        {
            nlohmann::json value = array[i++];
            ASSERT_TRUE(value.is_string());
            ASSERT_EQ("bar", value);
        }
    }

    {
        nlohmann::json array = json["sint_vec2"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(2, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }

    {
        nlohmann::json array = json["sint_vec3"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(3, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }

    {
        nlohmann::json array = json["sint_vec4"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(4, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }

    {
        nlohmann::json array = json["sint_vec"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(5, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec2"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(2, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec3"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(3, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec4"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(4, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }

    {
        nlohmann::json array = json["uint_vec"];
        ASSERT_TRUE(array.is_array());
        ASSERT_EQ(5, array.size());

        for (size_t i = 0; i < array.size(); i++) {
            ASSERT_TRUE(array[i].is_number_integer());
            ASSERT_EQ(i + 1, array[i]);
        }
    }
}

#endif // #if FLEXI_FEATURE_JSON
