#include "flexic.h"

#include <vector>

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

extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t len)
{
    TestStack stackImpl;
    TestStream streamImpl;

    flexi_stack_s stack =
        flexi_make_stack(TestStack::AtFunc, TestStack::CountFunc,
            TestStack::PushFunc, TestStack::PopFunc, &stackImpl);
    flexi_ostream_s ostream = flexi_make_ostream(TestStream::WriteFunc,
        TestStream::DataAtFunc, TestStream::TellFunc, &streamImpl);
    flexi_writer_s writer = flexi_make_writer(&stack, &ostream, NULL, NULL);

    flexi_ssize_t count = 0;
    for (size_t i = 0; i < len; i++) {
        switch (data[i]) {
        case 0x00:
            flexi_write_null(&writer, "null");
            count += 1;
            break;
        case 0x01:
            flexi_write_sint(&writer, "sint", 1);
            count += 1;
            break;
        case 0x02:
            flexi_write_uint(&writer, "uint", 2);
            count += 1;
            break;
        case 0x03:
            flexi_write_f32(&writer, "f32", 3.0f);
            count += 1;
            break;
        case 0x04:
            flexi_write_f64(&writer, "f64", 4.0);
            count += 1;
            break;
        case 0x05:
            flexi_write_key(&writer, "five");
            count += 1;
            break;
        case 0x06:
            flexi_write_string(&writer, "string", "string", 6);
            count += 1;
            break;
        case 0x07:
            flexi_write_indirect_sint(&writer, "in_sint", 7);
            count += 1;
            break;
        case 0x08:
            flexi_write_indirect_uint(&writer, "in_uint", 8);
            count += 1;
            break;
        case 0x09:
            flexi_write_indirect_f32(&writer, "f32", 9.0f);
            count += 1;
            break;
        case 0x0a:
            flexi_write_indirect_f64(&writer, "f64", 10.0);
            count += 1;
            break;
        case 0xFF: flexi_write_finalize(&writer); break;
        case 0xFE:
            flexi_write_vector(&writer, "vector", count, FLEXI_WIDTH_1B);
            count = 0;
            break;
        case 0xFD:
            flexi_write_map(&writer, "map", count, FLEXI_WIDTH_1B);
            count = 0;
            break;
        default: return -1;
        }
    }

    return 0;
}

extern "C" size_t
LLVMFuzzerMutate(uint8_t *data, size_t size, size_t maxSize);

extern "C" size_t
LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t maxSize,
    unsigned int seed)
{
    if (seed % 8 && size < maxSize) {
        size += 1;
    }

    if (size > 0) {
        data[size - 1] = 0xFF;
    }

    if (size > 1) {
        LLVMFuzzerMutate(data, size - 1, maxSize);
        for (size_t i = 0; i < size - 1; i++) {
            if (data[i] < 0xFD) {
                // Constrain fuzz to only valid cases.
                data[i] %= 0x0b;
            }
        }
    }

    return size;
}
