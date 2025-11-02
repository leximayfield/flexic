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

#include "flexic.h"

TEST(Writer, WriteInlineMapInts) {
    TestStream stream;

    {
        flexi_writer_s writer{};

        writer.user = &stream;
        writer.write_func = TestStream::WriteFunc;
        writer.data_at_func = TestStream::DataAtFunc;
        writer.tell_func = TestStream::TellFunc;

        ASSERT_TRUE(flexi_write_key(&writer, "bool"));
        ASSERT_TRUE(flexi_write_bool(&writer, true));
        ASSERT_TRUE(flexi_write_key(&writer, "sint"));
        ASSERT_TRUE(flexi_write_sint(&writer, INT16_MAX));
        ASSERT_TRUE(flexi_write_key(&writer, "indirect_sint"));
        ASSERT_TRUE(flexi_write_indirect_sint(&writer, INT32_MAX));
        ASSERT_TRUE(flexi_write_key(&writer, "uint"));
        ASSERT_TRUE(flexi_write_uint(&writer, UINT16_MAX));
        ASSERT_TRUE(flexi_write_key(&writer, "indirect_uint"));
        ASSERT_TRUE(flexi_write_indirect_uint(&writer, UINT32_MAX));
        ASSERT_TRUE(flexi_write_inline_map(&writer, 5, FLEXI_WIDTH_2B));
        ASSERT_TRUE(flexi_write_finalize(&writer));
    }
}
