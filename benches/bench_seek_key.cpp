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
flexic_SeekMap50(const std::string &str)
{
    flexi_cursor_s cursor, cursorTwo, cursorThree;

    flexi_buffer_s view = flexi_make_buffer(str.data(), str.length());

    flexi_result_e res;
    res = flexi_open_buffer(&view, &cursor);
    assert(res == FLEXI_OK);
    res = flexi_cursor_seek_map_key(&cursor, "map-50", &cursorTwo);
    assert(res == FLEXI_OK);
    res = flexi_cursor_seek_map_key(&cursorTwo, "key-50", &cursorThree);
    assert(res == FLEXI_OK);

    const char *value = nullptr;
    flexi_ssize_t len = -1;
    flexi_cursor_string(&cursorThree, &value, &len);
    assert(!strcmp(str.c_str(), "v-50-50"));
    return value;
}

/******************************************************************************/

static const char *
flatbuffers_SeekMap50(const std::string &str)
{
    auto rootRef = flexbuffers::GetRoot(
        reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
    auto mapRef = rootRef.AsMap()["map-50"];
    auto valueRef = mapRef.AsMap()["key-50"];
    auto value = valueRef.AsString();
    assert(!strcmp(value.c_str(), "v-50-50"));
    return value.c_str();
}

/******************************************************************************/

static const char *
yyjson_SeekMap50(const std::string &str)
{
    yyjson_doc *doc = yyjson_read(str.c_str(), str.length(), 0);
    assert(doc);
    yyjson_val *rootVal = yyjson_doc_get_root(doc);
    assert(rootVal);
    yyjson_val *mapVal = yyjson_obj_get(rootVal, "map-50");
    assert(mapVal);
    yyjson_val *keyVal = yyjson_obj_get(mapVal, "key-50");
    assert(keyVal);
    const char *value = yyjson_get_str(keyVal);
    assert(!strcmp(value, "v-50-50"));
    yyjson_doc_free(doc);
    return value;
}

/******************************************************************************/

void
bench_BenchSeekKey()
{
    std::string flexbuf_doc = bench_ReadFileToString("large_doc1.flexbuf");
    std::string json_doc = bench_ReadFileToString("large_doc1.json");

    auto bench = ankerl::nanobench::Bench()
                     .minEpochTime(std::chrono::milliseconds{500})
                     .title("Seek value of root[map-50][key-50]");

    bench.run("leximayfield/flexic",
        [&] { return flexic_SeekMap50(flexbuf_doc); });

    bench.run("google/flatbuffers",
        [&] { return flatbuffers_SeekMap50(flexbuf_doc); });

    bench.run("ibireme/yyjson.h", [&] { return yyjson_SeekMap50(json_doc); });
}
