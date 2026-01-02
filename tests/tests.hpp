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

#pragma once

#include "flexic.h"

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

CATCH_REGISTER_ENUM(flexi_result_e);
CATCH_REGISTER_ENUM(flexi_width_e);
CATCH_REGISTER_ENUM(flexi_type_e);

#include <array>
#include <vector>

using namespace Catch::Matchers;

#define INT8_PATTERN (INT8_C(-120))
#define INT16_PATTERN (INT16_C(-26232))
#define INT32_PATTERN (INT32_C(-1146447480))
#define INT64_PATTERN (INT64_C(-4822678189205112))

#define UINT8_PATTERN (UINT8_C(0x88))
#define UINT16_PATTERN (UINT16_C(0x9988))
#define UINT32_PATTERN (UINT32_C(0xbbaa9988))
#define UINT64_PATTERN (UINT64_C(0xffeeddccbbaa9988))

#define PI_VALUE_DBL (3.14159265358979323846)
#define PI_VALUE_FLT (3.14159265358979323846f)

enum class direct_e {
    direct,
    indirect,
};

class TestStream {
    std::vector<uint8_t> m_buffer;

public:
    bool Write(const void *ptr, flexi_ssize_t len)
    {
        auto bytes = static_cast<const uint8_t *>(ptr);
        m_buffer.insert(m_buffer.end(), bytes, bytes + len);
        return true;
    }

    const uint8_t *DataAt(flexi_ssize_t index) const
    {
        return &m_buffer[index];
    }

    bool Tell(flexi_ssize_t *offset) const
    {
        *offset = flexi_ssize_t(m_buffer.size());
        return true;
    }

    static bool WriteFunc(const void *ptr, flexi_ssize_t len, void *user)
    {
        auto stream = static_cast<TestStream *>(user);
        return stream->Write(ptr, len);
    }

    static const void *DataAtFunc(flexi_ssize_t index, void *user)
    {
        auto stream = static_cast<TestStream *>(user);
        return stream->DataAt(index);
    }

    static bool TellFunc(flexi_ssize_t *offset, void *user)
    {
        auto stream = static_cast<TestStream *>(user);
        return stream->Tell(offset);
    }
};

class TestStack {
    std::vector<flexi_value_s> m_buffer;

public:
    static flexi_value_s *AtFunc(flexi_stack_idx_t offset, void *user)
    {
        auto stack = static_cast<TestStack *>(user);
        if (offset < 0 || offset >= stack->m_buffer.size()) {
            return nullptr;
        }
        return &stack->m_buffer[offset];
    }
    static flexi_ssize_t CountFunc(void *user)
    {
        auto stack = static_cast<TestStack *>(user);
        return ptrdiff_t(stack->m_buffer.size());
    }

    static flexi_value_s *PushFunc(void *user)
    {
        auto stack = static_cast<TestStack *>(user);
        stack->m_buffer.push_back({});
        return &stack->m_buffer.back();
    }

    static ptrdiff_t PopFunc(ptrdiff_t count, void *user)
    {
        auto stack = static_cast<TestStack *>(user);

        ptrdiff_t start_size = ptrdiff_t(stack->m_buffer.size());
        ptrdiff_t wanted_size = start_size - count;
        if (wanted_size < 0) {
            stack->m_buffer.clear();
            return start_size;
        }

        stack->m_buffer.resize(size_t(wanted_size));
        return count;
    }
};

/**
 * @brief A fixture for writing and checking written data.
 */
class TestWriter {
protected:
    TestStream m_actual;
    TestStack m_stack;
    flexi_writer_s m_writer{};

public:
    TestWriter()
    {
        flexi_stack_s stack =
            flexi_make_stack(TestStack::AtFunc, TestStack::CountFunc,
                TestStack::PushFunc, TestStack::PopFunc, &m_stack);
        flexi_ostream_s ostream = flexi_make_ostream(TestStream::WriteFunc,
            TestStream::DataAtFunc, TestStream::TellFunc, &m_actual);
        m_writer = flexi_make_writer(&stack, &ostream, NULL, NULL);
    }

    ~TestWriter() { flexi_destroy_writer(&m_writer); }

    void AssertData(const std::vector<uint8_t> &expected) const
    {
        flexi_ssize_t actual_size = 0;
        REQUIRE(m_actual.Tell(&actual_size));

        // If the size is bad, we still want to know where any misalignment
        // is located in the buffer.
        CHECK(expected.size() == actual_size);
        flexi_ssize_t min_size =
            std::min(flexi_ssize_t(expected.size()), actual_size);

        for (flexi_ssize_t i = 0; i < min_size; i++) {
            CAPTURE(i);
            CHECK(expected[i] == *m_actual.DataAt(i));
        }
    }

    void GetCursor(flexi_cursor_s *cursor)
    {
        flexi_ssize_t offset = 0;
        REQUIRE(m_actual.Tell(&offset));

        auto span = flexi_make_span(m_actual.DataAt(0), offset);
        REQUIRE(FLEXI_OK == flexi_open_span(&span, cursor));
    }

    TestStream &GetActual() { return m_actual; }
    flexi_writer_s *GetWriter() { return &m_writer; }
};

/**
 * @brief A fixture for writing and checking written data - with strdup.
 */
class TestWriterStrdup : public TestWriter {
public:
    TestWriterStrdup()
    {
        flexi_stack_s stack =
            flexi_make_stack(TestStack::AtFunc, TestStack::CountFunc,
                TestStack::PushFunc, TestStack::PopFunc, &m_stack);
        flexi_ostream_s ostream = flexi_make_ostream(TestStream::WriteFunc,
            TestStream::DataAtFunc, TestStream::TellFunc, &m_actual);
        m_writer = flexi_make_writer(&stack, &ostream, strdup, free);
    }
};

/**
 * @brief Read a file into a string as binary.
 *
 * @param filename Filename to read.
 * @return String containing binary data.
 */
std::string
ReadFileToString(const char *filename);
