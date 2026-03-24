#pragma once

#include "metalcraft/RenderTypes.h"

#include <cstddef>
#include <functional>

namespace metalcraft {

struct PreparedIndirectCommands {
    const IndirectDrawArraysCommand* commands = nullptr;
    std::size_t commandCount = 0;
    std::size_t stride = sizeof(IndirectDrawArraysCommand);
    std::size_t byteLength = 0;
    std::size_t ringOffset = 0;
    bool stagedInRing = false;
    bool zeroCopy = false;

    bool valid() const noexcept {
        return commands != nullptr && commandCount > 0;
    }
};

using IndirectCommandAllocator =
    std::function<bool(std::size_t size, std::size_t alignment, RingAllocation& allocation)>;

bool PrepareIndirectCommands(const void* source, std::size_t drawCount, std::size_t stride,
                             const IndirectCommandAllocator& allocator,
                             PreparedIndirectCommands& prepared);

} // namespace metalcraft
