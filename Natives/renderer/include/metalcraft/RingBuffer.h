#pragma once

#include "metalcraft/RenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace metalcraft {

class RingBuffer {
public:
    explicit RingBuffer(std::size_t capacity);

    bool allocate(std::size_t size, std::size_t alignment, RingAllocation& allocation);
    void beginFrame();

    std::size_t capacity() const noexcept;
    std::size_t cursor() const noexcept;
    std::size_t frameIndex() const noexcept;
    std::uint8_t* data() noexcept;
    const std::uint8_t* data() const noexcept;

private:
    std::vector<std::uint8_t> storage_;
    std::size_t cursor_ = 0;
    std::size_t frameIndex_ = 0;
};

} // namespace metalcraft
