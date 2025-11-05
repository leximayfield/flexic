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

#include "flexic.h"

#include "gtest/gtest.h"

#include <array>
#include <iostream>

#define INT8_PATTERN (INT8_C(-120))
#define INT16_PATTERN (INT16_C(-26232))
#define INT32_PATTERN (INT32_C(-1146447480))
#define INT64_PATTERN (INT64_C(-4822678189205112))

#define UINT8_PATTERN (UINT8_C(0x88))
#define UINT16_PATTERN (UINT16_C(0x8899))
#define UINT32_PATTERN (UINT32_C(0x8899aabb))
#define UINT64_PATTERN (UINT64_C(0x8899aabbccddeeff))

#define PI_VALUE (3.14159265358979323846)

enum class direct_e {
    direct,
    indirect,
};

class TestStream {
    std::array<uint8_t, 256> m_buffer{};
    size_t m_offset = 0;

public:
    bool Write(const void *ptr, size_t len) {
        if (m_offset + len >= std::size(m_buffer)) {
            return false;
        }

        memcpy(&m_buffer[m_offset], ptr, len);
        m_offset += len;
        return true;
    }

    const uint8_t *DataAt(size_t index) const { return &m_buffer[index]; }

    bool Tell(size_t *offset) const {
        *offset = m_offset;
        return true;
    }

    static bool WriteFunc(const void *ptr, size_t len, void *user) {
        auto stream = static_cast<TestStream *>(user);
        return stream->Write(ptr, len);
    }

    static const void *DataAtFunc(size_t index, void *user) {
        auto stream = static_cast<TestStream *>(user);
        return stream->DataAt(index);
    }

    static bool TellFunc(size_t *offset, void *user) {
        auto stream = static_cast<TestStream *>(user);
        return stream->Tell(offset);
    }
};

class WriteFixture : public testing::Test {
protected:
    TestStream m_actual;
    flexi_writer_s m_writer{};

    void SetUp() override {
        m_writer.user = &m_actual;
        m_writer.write_func = TestStream::WriteFunc;
        m_writer.data_at_func = TestStream::DataAtFunc;
        m_writer.tell_func = TestStream::TellFunc;
    }

    void AssertData(const std::vector<uint8_t> &expected) {
        size_t actual_size = 0;
        ASSERT_TRUE(m_actual.Tell(&actual_size));
        ASSERT_EQ(expected.size(), actual_size);

        for (size_t i = 0; i < expected.size(); i++) {
            SCOPED_TRACE(testing::Message() << "At pos: " << i);
            ASSERT_EQ(expected[i], *m_actual.DataAt(i));
        }
    }

    void GetCursor(flexi_cursor_s *cursor) {
        size_t offset = 0;
        ASSERT_TRUE(m_actual.Tell(&offset));

        auto buffer = flexi_make_buffer(m_actual.DataAt(0), offset);
        ASSERT_TRUE(flexi_buffer_open(&buffer, cursor));
    }
};
