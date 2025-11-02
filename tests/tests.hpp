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

#include "gtest/gtest.h"

#include <array>

#define PI_VALUE (3.14159265358979323846)

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
