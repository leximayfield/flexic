#include "flexic.h"
#include <cstddef>
#include <cstdint>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    flexi_span_s span = flexi_make_span(data, size);
    flexi_cursor_s cursor;
    if (flexi_open_span(&span, &cursor) != FLEXI_OK) {
        return 0;
    }

    flexi_parser_s parser{
        [](const char *, void *) -> bool { return true; },
        [](const char *, int64_t, void *) -> bool { return true; },
        [](const char *, uint64_t, void *) -> bool { return true; },
        [](const char *, float, void *) -> bool { return true; },
        [](const char *, double, void *) -> bool { return true; },
        [](const char *, const char *, void *) -> bool { return true; },
        [](const char *, const char *, flexi_ssize_t, void *) -> bool {
            return true;
        },
        [](const char *, flexi_ssize_t, void *) -> bool { return true; },
        [](void *) -> bool { return true; },
        [](const char *, flexi_ssize_t, void *) -> bool { return true; },
        [](void *) -> bool { return true; },
        [](const char *, const void *, flexi_type_e, int, flexi_ssize_t,
            void *) -> bool { return true; },
        [](const char *, const void *, flexi_ssize_t, void *) -> bool {
            return true;
        },
        [](const char *, bool, void *) -> bool { return true; },
    };

    (void)flexi_parse_cursor(&parser, &cursor, NULL);
    return 0; // non-zero reserved for future use
}
