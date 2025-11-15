#include "flexic.h"
#include <cstddef>
#include <cstdint>

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    flexi_buffer_s buffer = flexi_make_buffer(data, size);
    flexi_cursor_s cursor;
    if (flexi_open_buffer(&buffer, &cursor) != FLEXI_OK) {
        return 0;
    }

    flexi_parser_s parser = flexi_make_empty_parser();
    (void)flexi_parse_cursor(&parser, &cursor, NULL);
    return 0; // non-zero reserved for future use
}
