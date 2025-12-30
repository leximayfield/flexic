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

static const char *
flexic_SeekMap50(const flexi_cursor_s *cursor)
{
    flexi_cursor_s cursorTwo, cursorThree;

    flexi_result_e res;
    res = flexi_cursor_seek_map_key(cursor, "map-50", &cursorTwo);
    assert(res == FLEXI_OK);
    res = flexi_cursor_seek_map_key(&cursorTwo, "key-50", &cursorThree);
    assert(res == FLEXI_OK);

    const char *value = nullptr;
    flexi_ssize_t len = -1;
    flexi_cursor_string(&cursorThree, &value, &len);
    assert(!strcmp(value, "v-50-50"));
    return value;
}

/******************************************************************************/

static flexbuffers::String
flatbuffers_SeekMap50(const flexbuffers::Reference &rootRef)
{
    auto mapRef = rootRef.AsMap()["map-50"];
    auto valueRef = mapRef.AsMap()["key-50"];
    auto value = valueRef.AsString();
    assert(!strcmp(value.c_str(), "v-50-50"));
    return value;
}

/******************************************************************************/

static std::string
json_SeekMap50(const nlohmann::json &root)
{
    const nlohmann::json &key = root["map-50"]["key-50"];
    const std::string &value = key.get<std::string>();
    assert(!strcmp(value.c_str(), "v-50-50"));
    return value;
}

/******************************************************************************/

static const char *
yyjson_SeekMap50(yyjson_val *rootVal)
{
    yyjson_val *mapVal = yyjson_obj_get(rootVal, "map-50");
    assert(mapVal);
    yyjson_val *keyVal = yyjson_obj_get(mapVal, "key-50");
    assert(keyVal);
    const char *value = yyjson_get_str(keyVal);
    assert(!strcmp(value, "v-50-50"));
    return value;
}

/******************************************************************************/

void
bench_BenchSeekKey(const char *flexbuf, const char *json, const char *title)
{
    std::string flexbuf_doc = bench_ReadFileToString(flexbuf);
    std::string json_doc = bench_ReadFileToString(json);

    auto bench = ankerl::nanobench::Bench()
                     .minEpochTime(std::chrono::milliseconds{100})
                     .title(title);

    {
        flexi_cursor_s cursor = flexi_StringToRoot(flexbuf_doc);

        bench.run("leximayfield/flexic", [&] {
            ankerl::nanobench::doNotOptimizeAway(flexic_SeekMap50(&cursor));
        });
    }

    {
        flexbuffers::Reference rootRef = flatbuffers_StringToRoot(flexbuf_doc);

        bench.run("google/flatbuffers", [&] {
            ankerl::nanobench::doNotOptimizeAway(
                flatbuffers_SeekMap50(rootRef));
        });
    }

    {
        nlohmann::json root = json_StringToRoot(json_doc);

        bench.run("nlohmann/json", [&] {
            ankerl::nanobench::doNotOptimizeAway(json_SeekMap50(root));
        });
    }

    {
        yyjson_pair yypair = yyjson_StringToRoot(json_doc);

        bench.run("ibireme/yyjson.h", [&] {
            ankerl::nanobench::doNotOptimizeAway(
                yyjson_SeekMap50(yypair.yyroot));
        });
    }
}

/******************************************************************************/

void
bench_BenchParseSeekKey(const char *flexbuf, const char *json,
    const char *title)
{
    std::string flexbuf_doc = bench_ReadFileToString(flexbuf);
    std::string json_doc = bench_ReadFileToString(json);

    auto bench = ankerl::nanobench::Bench()
                     .minEpochTime(std::chrono::milliseconds{100})
                     .title(title);

    {
        bench.run("leximayfield/flexic", [&] {
            flexi_cursor_s cursor = flexi_StringToRoot(flexbuf_doc);
            ankerl::nanobench::doNotOptimizeAway(flexic_SeekMap50(&cursor));
        });
    }

    {

        bench.run("google/flatbuffers", [&] {
            flexbuffers::Reference rootRef =
                flatbuffers_StringToRoot(flexbuf_doc);
            ankerl::nanobench::doNotOptimizeAway(
                flatbuffers_SeekMap50(rootRef));
        });
    }

    {
        bench.run("nlohmann/json", [&] {
            nlohmann::json root = json_StringToRoot(json_doc);
            ankerl::nanobench::doNotOptimizeAway(json_SeekMap50(root));
        });
    }

    {
        bench.run("ibireme/yyjson.h", [&] {
            yyjson_pair yypair = yyjson_StringToRoot(json_doc);
            ankerl::nanobench::doNotOptimizeAway(
                yyjson_SeekMap50(yypair.yyroot));
        });
    }
}
