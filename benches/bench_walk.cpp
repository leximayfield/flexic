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

static bool
flexic_EmitString(const char *key, const char *str, flexi_ssize_t len, void *)
{
    volatile const char *k = key;
    volatile const char *s = str;
    volatile flexi_ssize_t l = len;
    return true;
}

static bool
flexic_EmitBeginMap(const char *key, flexi_ssize_t len, void *)
{
    volatile const char *k = key;
    volatile size_t l = len;
    return true;
}

static bool
flexic_EmitEndMap(void *)
{
    return true;
}

static bool
flexic_EmitTypedVector(const char *key, const void *ptr, flexi_type_e type,
    int width, flexi_ssize_t count, void *user)
{
    volatile const char *k = key;
    volatile const void *p = ptr;
    volatile flexi_type_e t = type;
    volatile int w = width;
    volatile flexi_ssize_t c = count;
    return true;
}

static flexi_parser_s
flexic_Parser()
{
    flexi_parser_s parser;
    parser.map_begin = flexic_EmitBeginMap;
    parser.map_end = flexic_EmitEndMap;
    parser.string = flexic_EmitString;
    parser.typed_vector = flexic_EmitTypedVector;
    return parser;
}

/******************************************************************************/

static void
bench_EmitFloat(const char *key, float value)
{
    volatile const char *k = key;
    volatile float v = value;
}

static void
bench_EmitString(const char *key, const char *str, size_t len)
{
    volatile const char *k = key;
    volatile const char *s = str;
    volatile size_t l = len;
}

static void
bench_EmitBeginIter(const char *key, size_t len)
{
    volatile const char *k = key;
    volatile size_t l = len;
}

static void
bench_EmitEndIter()
{
}

static bool
bench_EmitFloat3(const char *key, const void *ptr, flexbuffers::Type type)
{
    volatile const char *k = key;
    volatile const void *p = ptr;
    volatile flexbuffers::Type t = type;
    return true;
}

/******************************************************************************/

static int
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
        bench_EmitString(key, str.c_str(), str.length());
        break;
    }
    case flexbuffers::FBT_INDIRECT_INT: assert(false); break;
    case flexbuffers::FBT_INDIRECT_UINT: assert(false); break;
    case flexbuffers::FBT_INDIRECT_FLOAT: assert(false); break;
    case flexbuffers::FBT_MAP: {
        flexbuffers::Map map = ref.AsMap();
        bench_EmitBeginIter(key, map.size());
        for (size_t i = 0; i < map.size(); i++) {
            const char *key = map.Keys()[i].AsKey();
            const auto &value = map.Values()[i];
            flatbuffers_WalkValue(key, value);
        }
        bench_EmitEndIter();
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
    case flexbuffers::FBT_VECTOR_FLOAT3: {
        // [LM] Why in the world does Google's official implementation not
        //      allow direct access to the pointer???  We also don't know
        //      what the underlying width is???
        auto r = ref.AsFixedTypedVector();
        float data[3] = {r[0].AsFloat(), r[1].AsFloat(), r[2].AsFloat()};
        bench_EmitFloat3(key, &data[0], ref.GetType());
        break;
    }
    case flexbuffers::FBT_VECTOR_INT4: assert(false); break;
    case flexbuffers::FBT_VECTOR_UINT4: assert(false); break;
    case flexbuffers::FBT_VECTOR_FLOAT4: assert(false); break;
    case flexbuffers::FBT_BLOB: assert(false); break;
    case flexbuffers::FBT_BOOL: assert(false); break;
    case flexbuffers::FBT_VECTOR_BOOL: assert(false); break;
    }

    return 0;
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

static int
json_Parser(const char *key, nlohmann::json &value)
{
    switch (value.type()) {
    case nlohmann::json::value_t::null: assert(false); break;
    case nlohmann::json::value_t::object:
        bench_EmitBeginIter(key, value.size());

        for (auto pairs : value.items()) {
            pairs.key();
            pairs.value();
            json_Parser(pairs.key().c_str(), pairs.value());
        }

        bench_EmitEndIter();
        break;
    case nlohmann::json::value_t::array:
        bench_EmitBeginIter(key, value.size());

        for (auto pairs : value.items()) {
            pairs.key();
            pairs.value();
            json_Parser(pairs.key().c_str(), pairs.value());
        }

        bench_EmitEndIter();
        break;
    case nlohmann::json::value_t::string: {
        const std::string &str = value.get<std::string>();
        bench_EmitString(key, str.data(), str.length());
        break;
    }
    case nlohmann::json::value_t::boolean: assert(false); break;
    case nlohmann::json::value_t::number_integer: assert(false); break;
    case nlohmann::json::value_t::number_unsigned: assert(false); break;
    case nlohmann::json::value_t::number_float:
        bench_EmitFloat(key, value.get<float>());
        break;
    case nlohmann::json::value_t::binary: assert(false); break;
    case nlohmann::json::value_t::discarded: assert(false); break;
    }
    return 0;
}

/******************************************************************************/

class json_SAXParser : public nlohmann::json::json_sax_t {
public:
    virtual bool null()
    {
        assert(false);
        return true;
    }
    virtual bool boolean(bool val)
    {
        assert(false);
        return true;
    }

    virtual bool number_integer(number_integer_t val)
    {
        assert(false);
        return true;
    }

    virtual bool number_unsigned(number_unsigned_t val)
    {
        assert(false);
        return true;
    }

    virtual bool number_float(number_float_t val, const string_t &s)
    {
        double value = std::stod(s);
        volatile float v = float(value);
        return true;
    }

    virtual bool string(string_t &val)
    {
        volatile const char *v = val.c_str();
        volatile size_t l = val.length();
        return true;
    }

    virtual bool binary(binary_t &val)
    {
        assert(false);
        return true;
    }

    virtual bool start_object(std::size_t elements)
    {
        volatile size_t e = elements;
        return true;
    }

    virtual bool key(string_t &val)
    {
        volatile const char *v = val.c_str();
        return true;
    }

    virtual bool end_object() { return true; }

    virtual bool start_array(std::size_t elements)
    {
        volatile size_t e = elements;
        return true;
    }

    virtual bool end_array() { return true; }

    virtual bool parse_error(std::size_t position,
        const std::string &last_token, const nlohmann::json::exception &ex)
    {
        assert(false);
        return true;
    }
};

/******************************************************************************/

static int
yyjson_WalkValue(const char *key, yyjson_val *value)
{
    switch (yyjson_get_type(value)) {
    case YYJSON_TYPE_RAW: assert(false); break;
    case YYJSON_TYPE_NULL: assert(false); break;
    case YYJSON_TYPE_BOOL: assert(false); break;
    case YYJSON_TYPE_NUM: {
        double flt = yyjson_get_num(value);
        bench_EmitFloat(key, float(flt));
        break;
    }
    case YYJSON_TYPE_STR: {
        const char *str = yyjson_get_str(value);
        size_t len = yyjson_get_len(value);
        bench_EmitString(key, str, len);
        break;
    }
    case YYJSON_TYPE_ARR: {
        bench_EmitBeginIter(key, yyjson_arr_size(value));
        size_t idx, max;
        yyjson_val *val;
        yyjson_arr_foreach(value, idx, max, val)
        {
            yyjson_WalkValue(NULL, val);
        }
        bench_EmitEndIter();
    }
    case YYJSON_TYPE_OBJ: {
        bench_EmitBeginIter(key, yyjson_obj_size(value));
        size_t idx, imax;
        yyjson_val *ikey, *ival;
        yyjson_obj_foreach(value, idx, imax, ikey, ival)
        {
            const char *keyStr = yyjson_get_str(ikey);
            yyjson_WalkValue(keyStr, ival);
        }
        bench_EmitEndIter();
        break;
    }
    }

    return 0;
}

/******************************************************************************/

void
bench_BenchWalk(const char *flexbuf, const char *json, const char *title)
{
    std::string flexbuf_doc = bench_ReadFileToString(flexbuf);
    std::string json_doc = bench_ReadFileToString(json);

    auto bench = ankerl::nanobench::Bench()
                     .minEpochTime(std::chrono::milliseconds{100})
                     .title(title);

    {
        flexi_parser_s parser = flexic_Parser();
        flexi_cursor_s cursor = flexi_StringToRoot(flexbuf_doc);

        bench.run("leximayfield/flexic", [&] {
            flexi_result_e res = flexi_parse_cursor(&parser, &cursor, NULL);
            assert(FLEXI_SUCCESS(res));
            ankerl::nanobench::doNotOptimizeAway(res);
        });
    }

    {
        flexbuffers::Reference rootRef = flatbuffers_StringToRoot(flexbuf_doc);

        bench.run("google/flatbuffers", [&] {
            ankerl::nanobench::doNotOptimizeAway(
                flatbuffers_WalkValue(NULL, rootRef));
        });
    }

    {
        // [LM] Don't use SAX parser - it only works on strings.
        nlohmann::json root = json_StringToRoot(json_doc);

        bench.run("nlohmann/json (manual)", [&] {
            ankerl::nanobench::doNotOptimizeAway(json_Parser(NULL, root));
        });
    }

    {
        yyjson_pair yypair = yyjson_StringToRoot(json_doc);

        bench.run("ibireme/yyjson.h", [&] {
            ankerl::nanobench::doNotOptimizeAway(
                yyjson_WalkValue(NULL, yypair.yyroot));
        });
    }
}

/******************************************************************************/

void
bench_BenchParseWalk(const char *flexbuf, const char *json, const char *title)
{
    std::string flexbuf_doc = bench_ReadFileToString(flexbuf);
    std::string json_doc = bench_ReadFileToString(json);

    auto bench = ankerl::nanobench::Bench()
                     .minEpochTime(std::chrono::milliseconds{100})
                     .title(title);

    {
        bench.run("leximayfield/flexic", [&] {
            flexi_parser_s parser = flexic_Parser();
            flexi_cursor_s cursor = flexi_StringToRoot(flexbuf_doc);
            flexi_result_e res = flexi_parse_cursor(&parser, &cursor, NULL);
            assert(FLEXI_SUCCESS(res));
            ankerl::nanobench::doNotOptimizeAway(res);
        });
    }

    {
        bench.run("google/flatbuffers", [&] {
            flexbuffers::Reference rootRef =
                flatbuffers_StringToRoot(flexbuf_doc);
            ankerl::nanobench::doNotOptimizeAway(
                flatbuffers_WalkValue(NULL, rootRef));
        });
    }

    {
        bench.run("nlohmann/json (sax_parse)", [&] {
            // [LM] Now we can use the SAX parser.
            json_SAXParser sax;
            ankerl::nanobench::doNotOptimizeAway(
                nlohmann::json::sax_parse<const char *, json_SAXParser>(
                    json_doc.c_str(), &sax));
        });
    }

    {
        bench.run("ibireme/yyjson.h", [&] {
            yyjson_pair yypair = yyjson_StringToRoot(json_doc);
            ankerl::nanobench::doNotOptimizeAway(
                yyjson_WalkValue(NULL, yypair.yyroot));
        });
    }
}
