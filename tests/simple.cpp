#include "gtest/gtest.h"

#include "flexic.h"

TEST(Simple, SingleInt1)
{
    const uint8_t data[3] = {0x01, 0x04, 0x01};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_INT, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_stride(&cursor));

    {
        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_int(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        uint64_t v = 0;
        ASSERT_TRUE(flexi_cursor_uint(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        float v = 0.0f;
        ASSERT_TRUE(flexi_cursor_float(&cursor, &v));
        ASSERT_EQ(v, 1.0f);
    }

    {
        double v = 0.0;
        ASSERT_TRUE(flexi_cursor_double(&cursor, &v));
        ASSERT_EQ(v, 1.0);
    }
}

TEST(Simple, SingleUint1)
{
    const uint8_t data[3] = {0x01, 0x08, 0x01};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_UINT, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_stride(&cursor));

    {
        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_int(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        uint64_t v = 0;
        ASSERT_TRUE(flexi_cursor_uint(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        float v = 0.0f;
        ASSERT_TRUE(flexi_cursor_float(&cursor, &v));
        ASSERT_EQ(v, 1.0f);
    }

    {
        double v = 0.0;
        ASSERT_TRUE(flexi_cursor_double(&cursor, &v));
        ASSERT_EQ(v, 1.0);
    }
}

TEST(Simple, SingleFloat)
{
    const uint8_t data[6] = {0x00, 0x00, 0x80, 0x3F, 0x0E, 0x04};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_stride(&cursor));

    {
        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_int(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        uint64_t v = 0;
        ASSERT_TRUE(flexi_cursor_uint(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        float v = 0.0f;
        ASSERT_TRUE(flexi_cursor_float(&cursor, &v));
        ASSERT_EQ(v, 1.0f);
    }

    {
        double v = 0.0;
        ASSERT_TRUE(flexi_cursor_double(&cursor, &v));
        ASSERT_EQ(v, 1.0);
    }
}

TEST(Simple, SingleDouble)
{
    const uint8_t data[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x0f, 0x08};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_FLOAT, flexi_cursor_type(&cursor));
    ASSERT_EQ(8, flexi_cursor_stride(&cursor));

    {
        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_int(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        uint64_t v = 0;
        ASSERT_TRUE(flexi_cursor_uint(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        float v = 0.0f;
        ASSERT_TRUE(flexi_cursor_float(&cursor, &v));
        ASSERT_EQ(v, 1.0f);
    }

    {
        double v = 0.0;
        ASSERT_TRUE(flexi_cursor_double(&cursor, &v));
        ASSERT_EQ(v, 1.0);
    }
}

TEST(Simple, SingleString)
{
    const uint8_t data[23] = {0x12, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x66, 0x6C, 0x65, 0x78, 0x62,
                              0x75, 0x66, 0x66, 0x65, 0x72, 0x73, 0x21, 0x00, 0x13, 0x14, 0x01};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_STRING, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_stride(&cursor));

    size_t len = 0;
    ASSERT_TRUE(flexi_cursor_length(&cursor, &len));
    ASSERT_EQ(len, 18);

    const char *ch = nullptr;
    ASSERT_TRUE(flexi_cursor_string(&cursor, &ch));
    ASSERT_STREQ(ch, "hello flexbuffers!");
}

TEST(Simple, SingleIndirectInt1)
{
    const uint8_t data[4] = {0x01, 0x01, 0x18, 0x01};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_INDIRECT_INT, flexi_cursor_type(&cursor));
    ASSERT_EQ(1, flexi_cursor_stride(&cursor));

    {
        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_int(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        uint64_t v = 0;
        ASSERT_TRUE(flexi_cursor_uint(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        float v = 0.0f;
        ASSERT_TRUE(flexi_cursor_float(&cursor, &v));
        ASSERT_EQ(v, 1.0f);
    }

    {
        double v = 0.0;
        ASSERT_TRUE(flexi_cursor_double(&cursor, &v));
        ASSERT_EQ(v, 1.0);
    }
}

TEST(Simple, SingleIndirectFloat)
{
    const uint8_t data[7] = {0x00, 0x00, 0x80, 0x3f, 0x04, 0x22, 0x01};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_INDIRECT_FLOAT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_stride(&cursor));

    {
        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_int(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        uint64_t v = 0;
        ASSERT_TRUE(flexi_cursor_uint(&cursor, &v));
        ASSERT_EQ(v, 1);
    }

    {
        float v = 0.0f;
        ASSERT_TRUE(flexi_cursor_float(&cursor, &v));
        ASSERT_EQ(v, 1.0f);
    }

    {
        double v = 0.0;
        ASSERT_TRUE(flexi_cursor_double(&cursor, &v));
        ASSERT_EQ(v, 1.0);
    }
}

TEST(Simple, SimpleVector)
{
    const uint8_t data[22] = {0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
                              0x00, 0x0A, 0x00, 0x01, 0x00, 0x06, 0x06, 0x06, 0x0F, 0x2A, 0x01};

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_VECTOR, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_stride(&cursor));

    size_t len = 0;
    ASSERT_TRUE(flexi_cursor_length(&cursor, &len));
    ASSERT_EQ(len, 3);

    const flexi_packed_t *types;
    ASSERT_TRUE(flexi_cursor_vector_types(&cursor, &types));

    int64_t checked[3] = {1, 256, 65546};
    for (size_t i = 0; i < len; i++)
    {
        ASSERT_EQ(types[i], 6);

        flexi_cursor_s index;
        ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, i, &index));

        int64_t v = 0;
        ASSERT_TRUE(flexi_cursor_int(&index, &v));
        EXPECT_EQ(v, checked[i]);
    }
}

TEST(Simple, SimpleTypedVector)
{
    const uint8_t data[19] = {0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01,
                              0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x0C, 0x2E, 0x01};

    flexi_cursor_s cursor{};
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    ASSERT_EQ(FLEXI_TYPE_VECTOR_INT, flexi_cursor_type(&cursor));
    ASSERT_EQ(4, flexi_cursor_stride(&cursor));

    size_t len = 0;
    ASSERT_TRUE(flexi_cursor_length(&cursor, &len));
    ASSERT_EQ(len, 3);

    const void *vec_ptr = nullptr;
    ASSERT_TRUE(flexi_cursor_typed_vector_data(&cursor, &vec_ptr));
    auto vec_data = static_cast<const int32_t *>(vec_ptr);
    EXPECT_EQ(vec_data[0], 1);
    EXPECT_EQ(vec_data[1], 256);
    EXPECT_EQ(vec_data[2], 65546);
}

TEST(Simple, SimpleMap)
{
    const uint8_t data[19] = {0x66, 0x6F, 0x6F, 0x00, 0x03, 0x62, 0x61, 0x72, 0x00, 0x01,
                              0x0A, 0x01, 0x01, 0x01, 0x09, 0x14, 0x02, 0x24, 0x01};

    flexi_cursor_s cursor{};
    flexi_buffer_s buffer = flexi_make_buffer(data, sizeof(data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    size_t len = 0;
    ASSERT_TRUE(flexi_cursor_length(&cursor, &len));
    ASSERT_EQ(len, 1);

    const flexi_packed_t *types = nullptr;
    ASSERT_TRUE(flexi_cursor_vector_types(&cursor, &types));
    ASSERT_EQ(*types, 0x14);

    const char *key = nullptr;
    ASSERT_TRUE(flexi_cursor_map_key_at_index(&cursor, 0, &key));
    ASSERT_STREQ(key, "foo");

    flexi_cursor_s cursor_value{};
    ASSERT_TRUE(flexi_cursor_seek_vector_index(&cursor, 0, &cursor_value));

    const char *value = nullptr;
    ASSERT_TRUE(flexi_cursor_string(&cursor_value, &value));
    ASSERT_STREQ(value, "bar");
}
