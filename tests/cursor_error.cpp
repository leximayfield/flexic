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

#include "tests.hpp"

TEST(CursorError, OpenBuffer_TooSmall)
{
    flexi_buffer_s buffer;
    buffer.data = nullptr;
    buffer.length = 0;

    flexi_cursor_s cursor;
    ASSERT_EQ(FLEXI_ERR_BADREAD, flexi_open_buffer(&buffer, &cursor));
}

TEST(CursorError, OpenBuffer_ZeroWidth)
{
    std::array<uint8_t, 3> data{0x00, 0x00, 0x00};

    flexi_buffer_s buffer = flexi_make_buffer(data.data(), data.size());

    flexi_cursor_s cursor;
    ASSERT_EQ(FLEXI_ERR_BADREAD, flexi_open_buffer(&buffer, &cursor));
}
