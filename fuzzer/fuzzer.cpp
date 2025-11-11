#include "flexic.h"
#include <cstddef>
#include <cstdint>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    flexi_buffer_s buffer = flexi_make_buffer(data, size);
    flexi_cursor_s cursor;
    if (flexi_open_buffer(&buffer, &cursor) != FLEXI_OK)
    {
        return 0;
    }

    flexi_parser_s parser{
        [](const char *, void *) {},
        [](const char *, int64_t, void *) {},
        [](const char *, uint64_t, void *) {},
        [](const char *, float, void *) {},
        [](const char *, double, void *) {},
        [](const char *, const char *, void *) {},
        [](const char *, const char *, size_t, void *) {},
        [](const char *, size_t, void *) {},
        [](void *) {},
        [](const char *, size_t, void *) {},
        [](void *) {},
        [](const char *, const void *, size_t, flexi_type_e, int, void *) {},
        [](const char *, const void *, size_t, void *) {},
        [](const char *, bool, void *) {},
    };

    (void)flexi_parse_cursor(&parser, &cursor, NULL);
    return 0; // non-zero reserved for future use
}
