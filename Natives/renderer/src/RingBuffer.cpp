#include "metalcraft/RingBuffer.h"

#include <algorithm>

namespace metalcraft {
namespace {

std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept {
    const std::size_t safeAlignment = std::max<std::size_t>(alignment, 1U);
    const std::size_t remainder = value % safeAlignment;
    return remainder == 0 ? value : (value + safeAlignment - remainder);
}

} // namespace

RingBuffer::RingBuffer(std::size_t capacity)
    : storage_(std::max<std::size_t>(capacity, 1U), 0) {}

bool RingBuffer::allocate(std::size_t size, std::size_t alignment, RingAllocation& allocation) {
    if (size == 0 || size > storage_.size()) {
        return false;
    }

    std::size_t alignedCursor = alignUp(cursor_, alignment);
    if (alignedCursor + size > storage_.size()) {
        alignedCursor = 0;
    }
    if (alignedCursor + size > storage_.size()) {
        return false;
    }

    allocation.offset = alignedCursor;
    allocation.size = size;
    allocation.cpuAddress = storage_.data() + alignedCursor;
    cursor_ = alignedCursor + size;
    return true;
}

void RingBuffer::beginFrame() {
    cursor_ = 0;
    ++frameIndex_;
}

std::size_t RingBuffer::capacity() const noexcept {
    return storage_.size();
}

std::size_t RingBuffer::cursor() const noexcept {
    return cursor_;
}

std::size_t RingBuffer::frameIndex() const noexcept {
    return frameIndex_;
}

std::uint8_t* RingBuffer::data() noexcept {
    return storage_.data();
}

const std::uint8_t* RingBuffer::data() const noexcept {
    return storage_.data();
}

} // namespace metalcraft
