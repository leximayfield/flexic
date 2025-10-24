#include "gtest/gtest.h"

#include "flexic.h"

//
// Google's gold_flexbuffer_example.bin
//
//  {
//      'bar': [1, 2, 3],
//      'bar3': [1, 2, 3],
//      'bool': True,
//      'bools': [True, False, True, False],
//      'foo': 100.0,
//      'mymap': {'foo': 'Fred'},
//      'vec': [-100, 'Fred', 4.0, b'M', False, 4.0]
//  }
//

const uint8_t g_data[166] = {
    0x76, 0x65, 0x63, 0x00, 0x04, 0x46, 0x72, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x40, 0x01, 0x4D, 0x06,
    0x9C, 0x0F, 0x09, 0x05, 0x00, 0x0C, 0x04, 0x14, 0x22, 0x64, 0x68, 0x22, 0x62, 0x61, 0x72, 0x00, 0x00, 0x03, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x62, 0x61, 0x72, 0x33, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x62, 0x6F, 0x6F, 0x6C,
    0x73, 0x00, 0x04, 0x01, 0x00, 0x01, 0x00, 0x62, 0x6F, 0x6F, 0x6C, 0x00, 0x66, 0x6F, 0x6F, 0x00, 0x6D, 0x79, 0x6D,
    0x61, 0x70, 0x00, 0x01, 0x0B, 0x01, 0x01, 0x01, 0x62, 0x14, 0x07, 0x4B, 0x37, 0x19, 0x25, 0x16, 0x13, 0x70, 0x00,
    0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00, 0x48,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC8, 0x42, 0x2D, 0x00, 0x00, 0x00,
    0x85, 0x00, 0x00, 0x00, 0x2E, 0x4E, 0x6A, 0x90, 0x0E, 0x24, 0x28, 0x23, 0x26, 0x01};

TEST(Gold, MapLength)
{
    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(g_data, sizeof(g_data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    size_t len = 0;
    ASSERT_TRUE(flexi_cursor_length(&cursor, &len));
    ASSERT_EQ(len, 7);
}

TEST(Gold, CursorMapKeyAtIndex)
{
    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(g_data, sizeof(g_data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    const char *keys[7] = {"bar", "bar3", "bool", "bools", "foo", "mymap", "vec"};
    for (size_t i = 0; i < 7; i++)
    {
        const char *check = nullptr;
        ASSERT_TRUE(flexi_cursor_map_key_at_index(&cursor, i, &check));
        ASSERT_STREQ(check, keys[i]);
    }
}

TEST(Gold, SeekMapKey)
{
    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(g_data, sizeof(g_data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    const char *keys[7] = {"bar", "bar3", "bool", "bools", "foo", "mymap", "vec"};
    flexi_type_e types[7] = {
        FLEXI_TYPE_VECTOR_INT, FLEXI_TYPE_VECTOR_INT3, FLEXI_TYPE_BOOL,   FLEXI_TYPE_VECTOR_BOOL,
        FLEXI_TYPE_FLOAT,      FLEXI_TYPE_MAP,         FLEXI_TYPE_VECTOR,
    };

    for (size_t i = 0; i < 7; i++)
    {
        flexi_cursor_s check;
        ASSERT_TRUE(flexi_cursor_seek_map_key(&cursor, keys[i], &check));
        ASSERT_EQ(flexi_cursor_type(&check), types[i]);
    }
}

TEST(Gold, SeekMapKeyMissing)
{
    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(g_data, sizeof(g_data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));

    flexi_cursor_s found;
    ASSERT_FALSE(flexi_cursor_seek_map_key(&cursor, "plugh", &found));
}

TEST(Gold, Read)
{
    flexi_reader_s reader{
        [](int64_t) {},
        [](uint64_t) {},
        [](float) {},
        [](double) {},
        [](const char *, size_t) {},
        [](const void *, size_t) {},
        [](size_t) {},
        [](const char *) {},
        [](void) {},
        [](size_t) {},
        [](void) {},
        [](const int8_t *, size_t) {},
        [](const uint8_t *, size_t) {},
        [](const int16_t *, size_t) {},
        [](const uint16_t *, size_t) {},
        [](const int32_t *, size_t) {},
        [](const uint32_t *, size_t) {},
        [](const int64_t *, size_t) {},
        [](const uint64_t *, size_t) {},
        [](bool) {},
        [](const bool *, size_t) {},
    };

    flexi_cursor_s cursor;
    flexi_buffer_s buffer = flexi_make_buffer(g_data, sizeof(g_data));
    ASSERT_TRUE(flexi_buffer_open(&buffer, &cursor));
    ASSERT_TRUE(flexi_read(&reader, &cursor));
}
