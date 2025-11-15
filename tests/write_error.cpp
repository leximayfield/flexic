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

TEST(WriteError, FailSafe)
{
    flexi_writer_s writer{};
    writer.err = FLEXI_ERR_INTERNAL;

    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_null(&writer, NULL));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_sint(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_uint(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_f32(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_f64(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_key(&writer, ""));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_strlen(&writer, NULL, ""));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_indirect_sint(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_indirect_uint(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_indirect_f32(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_indirect_f64(&writer, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE,
        flexi_write_map_keys(&writer, 0, FLEXI_WIDTH_1B, NULL));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE,
        flexi_write_map(&writer, NULL, 0, 0, FLEXI_WIDTH_1B));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE,
        flexi_write_vector(&writer, NULL, 0, FLEXI_WIDTH_1B));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE,
        flexi_write_typed_vector_sint(&writer, NULL, NULL, FLEXI_WIDTH_1B, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE,
        flexi_write_typed_vector_uint(&writer, NULL, NULL, FLEXI_WIDTH_1B, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE,
        flexi_write_typed_vector_flt(&writer, NULL, NULL, FLEXI_WIDTH_1B, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_blob(&writer, NULL, NULL, 0, 1));

    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_bool(&writer, NULL, false));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE,
        flexi_write_typed_vector_bool(&writer, NULL, NULL, 0));
    EXPECT_EQ(FLEXI_ERR_FAILSAFE, flexi_write_finalize(&writer));
}
