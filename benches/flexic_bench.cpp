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

#include <fstream>
#include <sstream>

#include "flexic.h"

/******************************************************************************/

int
main(const int, const char *[])
{
    std::ifstream file{"large_doc1.flexbuf", std::ios::binary};
    std::stringstream stream;
    stream << file.rdbuf();
    std::string buffer = std::move(stream).str();

    flexi_buffer_s view = flexi_make_buffer(buffer.data(), buffer.length());

    flexi_cursor_s cursor;
    if (!flexi_open_buffer(&view, &cursor)) {
        throw "assert";
    }

    flexi_parser_s parser{
        [](void *) {},
        [](int64_t, void *) {},
        [](uint64_t, void *) {},
        [](float, void *) {},
        [](double, void *) {},
        [](const char *, void *) {},
        [](const char *, size_t, void *) {},
        [](const void *, size_t, void *) {},
        [](size_t, void *) {},
        [](const char *, void *) {},
        [](void *) {},
        [](size_t, void *) {},
        [](void *) {},
        [](const void *, int, size_t, void *) {},
        [](const void *, int, size_t, void *) {},
        [](const void *, int, size_t, void *) {},
        [](bool, void *) {},
        [](const bool *, size_t, void *) {},
    };

    flexi_cursor_s dest;
    ankerl::nanobench::Bench().run("flexi_cursor_seek_map_key",
        [&] { (void)flexi_cursor_seek_map_key(&cursor, "map-82", &dest); });
    ankerl::nanobench::Bench().run("flexi_read",
        [&] { (void)flexi_parse_cursor(&parser, &cursor, NULL); });
    return 0;
}
