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

TEST(ReadInt, Int)
{
    flexi_cursor_s cursor{};
    std::array<uint8_t, 3> data = {0x88, 0x04, 0x01};

    auto buffer = flexi_make_buffer(data.data(), data.size());
    ASSERT_TRUE(flexi_open_buffer(&buffer, &cursor));

    {
        ASSERT_EQ(FLEXI_TYPE_SINT, flexi_cursor_type(&cursor));
        ASSERT_EQ(1, flexi_cursor_width(&cursor));
        ASSERT_EQ(0, flexi_cursor_length(&cursor));
    }

    {
        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_sint(&cursor, &v));
        ASSERT_EQ(-120, v);
    }

    {
        uint64_t v = 0;
        ASSERT_TRUE(flexi_cursor_uint(&cursor, &v));
        ASSERT_EQ(136, v);
    }

    {
        float v = 0;
        ASSERT_TRUE(flexi_cursor_f32(&cursor, &v));
        ASSERT_EQ(-120.0f, v);
    }

    {
        double v = 0;
        ASSERT_TRUE(flexi_cursor_f64(&cursor, &v));
        ASSERT_EQ(-120.0, v);
    }

    {
        const char *v = nullptr;
        ASSERT_FALSE(flexi_cursor_key(&cursor, &v));
    }

    {
        const char *v = nullptr;
        ASSERT_FALSE(flexi_cursor_string(&cursor, &v));
    }

    {
        const void *v = nullptr;
        ASSERT_FALSE(flexi_cursor_typed_vector_data(&cursor, &v));
    }

    {
        const flexi_packed_t *v = nullptr;
        ASSERT_FALSE(flexi_cursor_vector_types(&cursor, &v));
    }

    {
        const uint8_t *v = nullptr;
        ASSERT_FALSE(flexi_cursor_blob(&cursor, &v));
    }

    {
        bool v = false;
        ASSERT_TRUE(flexi_cursor_bool(&cursor, &v));
        ASSERT_EQ(true, v);
    }

    {
        flexi_cursor_s v{};
        ASSERT_FALSE(flexi_cursor_seek_vector_index(&cursor, 0, &v));
    }

    {
        const char *v = nullptr;
        ASSERT_FALSE(flexi_cursor_map_key_at_index(&cursor, 0, &v));
    }

    {
        flexi_cursor_s v{};
        ASSERT_FALSE(flexi_cursor_seek_map_key(&cursor, "", &v));
    }
}
