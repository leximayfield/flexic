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

#pragma once

#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#elif defined(__GNUC__)
#define FORCEINLINE __attribute__((always_inline)) inline
#else
#define FORCEINLINE inline
#endif

#include "nanobench.h"

#include "flexic.h"
#include <flatbuffers/flexbuffers.h>
#include <nlohmann/json.hpp>
#include <yyjson.h>

FORCEINLINE flexi_cursor_s
flexi_StringToRoot(const std::string &str)
{
    flexi_span_s view = flexi_make_span(str.data(), str.length());

    flexi_cursor_s cursor;
    flexi_result_e res = flexi_open_span(&view, &cursor);
    assert(FLEXI_SUCCESS(res));
    return cursor;
}

FORCEINLINE flexbuffers::Reference
flatbuffers_StringToRoot(const std::string &str)
{
    return flexbuffers::GetRoot(reinterpret_cast<const uint8_t *>(str.c_str()),
        str.length());
}

FORCEINLINE nlohmann::json
json_StringToRoot(const std::string &str)
{
    return nlohmann::json::parse(str);
}

struct yyjson_pair {
    yyjson_doc *yydoc;
    yyjson_val *yyroot;

    yyjson_pair() = default;
    yyjson_pair(const yyjson_pair &) = delete;
    yyjson_pair(yyjson_pair &&other) noexcept
        : yydoc{std::exchange(other.yydoc, nullptr)},
          yyroot{std::exchange(other.yyroot, nullptr)}
    {
    }
    ~yyjson_pair() { yyjson_doc_free(yydoc); }
};

FORCEINLINE yyjson_pair
yyjson_StringToRoot(const std::string &str)
{
    yyjson_pair rvo;
    rvo.yydoc = yyjson_read(str.c_str(), str.length(), 0);
    assert(rvo.yydoc);
    rvo.yyroot = yyjson_doc_get_root(rvo.yydoc);
    assert(rvo.yyroot);
    return rvo;
}

std::string
bench_ReadFileToString(const char *filename);

void
bench_BenchSeekKey(const char *flexbuf, const char *json, const char *title);

void
bench_BenchParseSeekKey(const char *flexbuf, const char *json,
    const char *title);

void
bench_BenchWalk(const char *flexbuf, const char *json, const char *title);

void
bench_BenchParseWalk(const char *flexbuf, const char *json, const char *title);
