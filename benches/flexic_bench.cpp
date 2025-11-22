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

#include <flatbuffers/flexbuffers.h>

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
json_parse_and_walk(const std::string &str)
{
    json_parse_result_t result;

    json_value_s *root = json_parse_ex(str.c_str(), str.length(),
        json_parse_flags_default, NULL, NULL, &result);
    json_walk_value(NULL, root);
    free(root);

    return 0;
}

/******************************************************************************/

static json_object_element_s *
json_find_key(const json_object_s *obj, const char *name)
{
    json_object_element_s *element = obj->start;
    while (element) {
        if (!strcmp(element->name->string, name)) {
            return element;
        }
        element = element->next;
    }
    return nullptr;
}

/******************************************************************************/

static void
flexbuffers_emit_string(const char *key, const char *str, size_t len)
{
}

/******************************************************************************/

static void
flexbuffers_emit_begin_object(const char *key, size_t len)
{
}

/******************************************************************************/

static void
flexbuffers_emit_end_object()
{
}

/******************************************************************************/

static void
flexbuffers_walk_value(const char *key, const flexbuffers::Reference &ref)
{
    switch (ref.GetType()) {
    case flexbuffers::FBT_NULL: assert(false); break;
    case flexbuffers::FBT_INT: assert(false); break;
    case flexbuffers::FBT_UINT: assert(false); break;
    case flexbuffers::FBT_FLOAT: assert(false); break;
    case flexbuffers::FBT_KEY: assert(false); break;
    case flexbuffers::FBT_STRING: {
        flexbuffers::String str = ref.AsString();
        flexbuffers_emit_string(key, str.c_str(), str.length());
        break;
    }
    case flexbuffers::FBT_INDIRECT_INT: assert(false); break;
    case flexbuffers::FBT_INDIRECT_UINT: assert(false); break;
    case flexbuffers::FBT_INDIRECT_FLOAT: assert(false); break;
    case flexbuffers::FBT_MAP: {
        flexbuffers::Map map = ref.AsMap();
        flexbuffers_emit_begin_object(key, map.size());
        for (size_t i = 0; i < map.size(); i++) {
            const char *key = map.Keys()[i].AsKey();
            const auto &value = map.Values()[i];
            flexbuffers_walk_value(key, value);
        }
        flexbuffers_emit_end_object();
        break;
    }
    case flexbuffers::FBT_VECTOR: assert(false); break;
    case flexbuffers::FBT_VECTOR_INT: assert(false); break;
    case flexbuffers::FBT_VECTOR_UINT: assert(false); break;
    case flexbuffers::FBT_VECTOR_FLOAT: assert(false); break;
    case flexbuffers::FBT_VECTOR_KEY: assert(false); break;
    case flexbuffers::FBT_VECTOR_STRING_DEPRECATED: assert(false); break;
    case flexbuffers::FBT_VECTOR_INT2: assert(false); break;
    case flexbuffers::FBT_VECTOR_UINT2: assert(false); break;
    case flexbuffers::FBT_VECTOR_FLOAT2: assert(false); break;
    case flexbuffers::FBT_VECTOR_INT3: assert(false); break;
    case flexbuffers::FBT_VECTOR_UINT3: assert(false); break;
    case flexbuffers::FBT_VECTOR_FLOAT3: assert(false); break;
    case flexbuffers::FBT_VECTOR_INT4: assert(false); break;
    case flexbuffers::FBT_VECTOR_UINT4: assert(false); break;
    case flexbuffers::FBT_VECTOR_FLOAT4: assert(false); break;
    case flexbuffers::FBT_BLOB: assert(false); break;
    case flexbuffers::FBT_BOOL: assert(false); break;
    case flexbuffers::FBT_VECTOR_BOOL: assert(false); break;
    }
}

/******************************************************************************/

static size_t
flexbuffers_getroot_and_walk(const std::string &str)
{
    flexbuffers::Reference ref = flexbuffers::GetRoot(
        reinterpret_cast<const uint8_t *>(str.data()), str.length());
    flexbuffers_walk_value(NULL, ref);
    return 0;
}

/******************************************************************************/

int
main(const int, const char *[])
{
    std::string large_doc1_flexbuf = read_file_to_string("large_doc1.flexbuf");
    std::string large_doc1_json = read_file_to_string("large_doc1.json");

    {
        auto bench = ankerl::nanobench::Bench().minEpochIterations(16).title(
            "Seek value of root[map-50][key-50]");

        bench.run("sheredom/json.h", [&] {
            json_parse_result_t result;

            json_value_s *root =
                json_parse_ex(large_doc1_json.c_str(), large_doc1_json.length(),
                    json_parse_flags_default, NULL, NULL, &result);

            json_object_s *map = json_value_as_object(root);
            assert(map);
            json_object_element_s *mapele = json_find_key(map, "map-50");
            assert(mapele);
            json_object_s *obj = json_value_as_object(mapele->value);
            assert(obj);
            json_object_element_s *objele = json_find_key(obj, "key-50");
            assert(objele);
            json_string_s *str = json_value_as_string(objele->value);
            assert(!strcmp(str->string, "v-50-50"));
            free(root);
            return str;
        });

        bench.run("google/flatbuffers", [&] {
            auto rootRef = flexbuffers::GetRoot(
                reinterpret_cast<const uint8_t *>(large_doc1_flexbuf.c_str()),
                large_doc1_flexbuf.length());
            auto mapRef = rootRef.AsMap()["map-50"];
            auto valueRef = mapRef.AsMap()["key-50"];
            auto str = valueRef.AsString();
            assert(!strcmp(str.c_str(), "v-50-50"));
            return str;
        });

        bench.run("leximayfield/flexic", [&] {
            flexi_cursor_s cursor, cursorTwo, cursorThree;

            flexi_buffer_s view = flexi_make_buffer(large_doc1_flexbuf.data(),
                large_doc1_flexbuf.length());

            flexi_open_buffer(&view, &cursor);
            flexi_cursor_seek_map_key(&cursor, "map-50", &cursorTwo);
            flexi_cursor_seek_map_key(&cursorTwo, "key-50", &cursorThree);

            const char *str = nullptr;
            flexi_ssize_t len = -1;
            flexi_cursor_string(&cursorThree, &str, &len);
            assert(!strcmp(str, "v-50-50"));
            return str;
        });
    }

    {
        auto bench = ankerl::nanobench::Bench().minEpochIterations(16).title(
            "Walk entire document");

        bench.run("sheredom/json.h",
            [&] { return json_parse_and_walk(large_doc1_json); });

        bench.run("google/flatbuffers",
            [&] { return flexbuffers_getroot_and_walk(large_doc1_flexbuf); });

        flexi_parser_s parser = flexi_make_empty_parser();
        bench.run("leximayfield/flexic",
            [&] { return flexic_open_and_parse(large_doc1_flexbuf, parser); });
    }

    return 0;
}
