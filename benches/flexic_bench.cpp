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

#include "nanobench.h"

#include <cassert>
#include <fstream>
#include <sstream>

#include "flexic.h"

// [LM] Fastest maintained JSON library I could find here:
//      https://github.com/miloyip/nativejson-benchmark
#include "json.h"

/******************************************************************************/

static std::string
read_file_to_string(const char *filename)
{
    std::ifstream file{filename, std::ios::binary};
    std::stringstream stream;
    stream << file.rdbuf();
    return std::move(stream).str();
}

/******************************************************************************/

static flexi_result_e
flexic_open_and_parse(const std::string &str, const flexi_parser_s &parser)
{
    flexi_cursor_s cursor;

    flexi_buffer_s view = flexi_make_buffer(str.data(), str.length());

    flexi_result_e res = flexi_open_buffer(&view, &cursor);
    assert(FLEXI_SUCCESS(res));

    res = flexi_parse_cursor(&parser, &cursor, NULL);
    assert(FLEXI_SUCCESS(res));
    return res;
}

/******************************************************************************/

static void
json_emit_string(const char *key, const char *str, size_t len)
{
}

/******************************************************************************/

static void
json_emit_begin_object(const char *key, size_t len)
{
}

/******************************************************************************/

static void
json_emit_end_object()
{
}

/******************************************************************************/

static void
json_walk_value(const char *key, json_value_s *value)
{
    switch (value->type) {
    case json_type_string: {
        json_string_s *string = json_value_as_string(value);
        json_emit_string(key, string->string, string->string_size);
        break;
    }
    case json_type_number: assert(false); break;
    case json_type_object: {
        json_object_s *object = json_value_as_object(value);
        json_emit_begin_object(key, object->length);
        json_object_element_s *element = object->start;
        while (element) {
            json_walk_value(element->name->string, element->value);
            element = element->next;
        }
        json_emit_end_object();
        break;
    }
    case json_type_array: assert(false); break;
    case json_type_true: assert(false); break;
    case json_type_false: assert(false); break;
    case json_type_null: assert(false); break;
    }
}

/******************************************************************************/

static size_t
json_open_and_walk(const std::string &str)
{
    json_parse_result_t result;

    json_value_s *root =
        json_parse_ex(str.c_str(), str.length(), 0, NULL, NULL, &result);
    json_walk_value(NULL, root);
    free(root);

    return 4;
}

/******************************************************************************/

int
main(const int, const char *[])
{
    std::string large_doc1_flexbuf = read_file_to_string("large_doc1.flexbuf");
    std::string large_doc1_json = read_file_to_string("large_doc1.json");

    flexi_buffer_s view = flexi_make_buffer(large_doc1_flexbuf.data(),
        large_doc1_flexbuf.length());

    flexi_cursor_s cursor;
    if (FLEXI_ERROR(flexi_open_buffer(&view, &cursor))) {
        throw "assert";
    }

#if 0
    flexi_cursor_s dest;
    ankerl::nanobench::Bench().run("flexi_cursor_seek_map_key",
        [&] { (void)flexi_cursor_seek_map_key(&cursor, "map-82", &dest); });
#endif

    flexi_parser_s parser = flexi_make_empty_parser();
    ankerl::nanobench::Bench().run("flexi_parse_cursor",
        [&] { return flexic_open_and_parse(large_doc1_flexbuf, parser); });

    ankerl::nanobench::Bench().run("json_parse (sheredom/json.h)",
        [&] { return json_open_and_walk(large_doc1_json); });

    return 0;
}
