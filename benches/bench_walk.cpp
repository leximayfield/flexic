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

#include "flexic_bench.hpp"

/******************************************************************************/

static flexi_result_e
flexic_OpenAndParse(const std::string &str, const flexi_parser_s &parser)
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
flatbuffers_EmitString(const char *key, const char *str, size_t len)
{
}

static void
flatbuffers_EmitBeginObject(const char *key, size_t len)
{
}

static void
flatbuffers_EmitEndObject()
{
}

static void
flatbuffers_WalkValue(const char *key, const flexbuffers::Reference &ref)
{
    switch (ref.GetType()) {
    case flexbuffers::FBT_NULL: assert(false); break;
    case flexbuffers::FBT_INT: assert(false); break;
    case flexbuffers::FBT_UINT: assert(false); break;
    case flexbuffers::FBT_FLOAT: assert(false); break;
    case flexbuffers::FBT_KEY: assert(false); break;
    case flexbuffers::FBT_STRING: {
        flexbuffers::String str = ref.AsString();
        flatbuffers_EmitString(key, str.c_str(), str.length());
        break;
    }
    case flexbuffers::FBT_INDIRECT_INT: assert(false); break;
    case flexbuffers::FBT_INDIRECT_UINT: assert(false); break;
    case flexbuffers::FBT_INDIRECT_FLOAT: assert(false); break;
    case flexbuffers::FBT_MAP: {
        flexbuffers::Map map = ref.AsMap();
        flatbuffers_EmitBeginObject(key, map.size());
        for (size_t i = 0; i < map.size(); i++) {
            const char *key = map.Keys()[i].AsKey();
            const auto &value = map.Values()[i];
            flatbuffers_WalkValue(key, value);
        }
        flatbuffers_EmitEndObject();
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

static size_t
flatbuffers_GetRootAndWalk(const std::string &str)
{
    flexbuffers::Reference ref = flexbuffers::GetRoot(
        reinterpret_cast<const uint8_t *>(str.data()), str.length());
    flatbuffers_WalkValue(NULL, ref);
    return 0;
}

/******************************************************************************/

static void
yyjson_EmitString(const char *key, const char *str, size_t len)
{
}

static void
yyjson_EmitBeginObject(const char *key, size_t len)
{
}

static void
yyjson_EmitEndObject()
{
}

static void
yyjson_WalkValue(const char *key, yyjson_val *value)
{
    switch (yyjson_get_type(value)) {
    case YYJSON_TYPE_RAW: assert(false); break;
    case YYJSON_TYPE_NULL: assert(false); break;
    case YYJSON_TYPE_BOOL: assert(false); break;
    case YYJSON_TYPE_NUM: assert(false); break;
    case YYJSON_TYPE_STR: {
        const char *str = yyjson_get_str(value);
        size_t len = yyjson_get_len(value);
        yyjson_EmitString(key, str, len);
        break;
    }
    case YYJSON_TYPE_ARR: assert(false); break;
    case YYJSON_TYPE_OBJ: {
        yyjson_EmitBeginObject(key, yyjson_obj_size(value));
        size_t idx, imax;
        yyjson_val *ikey, *ival;
        yyjson_obj_foreach(value, idx, imax, ikey, ival)
        {
            const char *keyStr = yyjson_get_str(ikey);
            yyjson_WalkValue(keyStr, ival);
        }
        yyjson_EmitEndObject();
        break;
    }
    }
}

static size_t
yyjson_ParseAndWalk(const std::string &str)
{
    yyjson_doc *doc = yyjson_read(str.c_str(), str.length(), 0);
    yyjson_val *val = yyjson_doc_get_root(doc);
    yyjson_WalkValue(NULL, val);
    yyjson_doc_free(doc);
    return 0;
}

/******************************************************************************/

void
bench_BenchWalk()
{
    std::string flexbuf_doc = bench_ReadFileToString("large_doc1.flexbuf");
    std::string json_doc = bench_ReadFileToString("large_doc1.json");

    auto bench = ankerl::nanobench::Bench()
                     .minEpochTime(std::chrono::milliseconds{500})
                     .title("Walk entire document");

    flexi_parser_s parser = flexi_make_empty_parser();
    bench.run("leximayfield/flexic",
        [&] { return flexic_OpenAndParse(flexbuf_doc, parser); });

    bench.run("google/flatbuffers",
        [&] { return flatbuffers_GetRootAndWalk(flexbuf_doc); });

    bench.run("ibireme/yyjson.h",
        [&] { return yyjson_ParseAndWalk(json_doc); });
}
