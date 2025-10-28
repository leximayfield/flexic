#include "nanobench.h"

#include <fstream>
#include <sstream>

#include "flexic.h"

/******************************************************************************/

int main()
{
    std::ifstream file{"large_doc1.flexbuf", std::ios::binary};
    std::stringstream stream;
    stream << file.rdbuf();
    std::string buffer = std::move(stream).str();

    flexi_buffer_s view = flexi_make_buffer(buffer.data(), buffer.length());

    flexi_cursor_s cursor;
    if (!flexi_buffer_open(&view, &cursor))
    {
        throw "assert";
    }

    flexi_reader_s reader{
        [](int64_t) {},
        [](uint64_t) {},
        [](double) {},
        [](const char *, size_t) {},
        [](const void *, size_t) {},
        [](size_t) {},
        [](const char *) {},
        [](void) {},
        [](size_t) {},
        [](void) {},
        [](const void *, int, size_t) {},
        [](const void *, int, size_t) {},
        [](const void *, int, size_t) {},
        [](bool) {},
        [](const bool *, size_t) {},
    };

    flexi_cursor_s dest;
    ankerl::nanobench::Bench().run("flexi_cursor_seek_map_key",
                                   [&] { flexi_cursor_seek_map_key(&cursor, "map-82", &dest); });
    ankerl::nanobench::Bench().run("flexi_read", [&] { flexi_read(&reader, &cursor); });
    return 0;
}
