#include "metalcraft/IndirectCommandStream.h"

#include <cstdint>
#include <cstring>
#include <limits>

namespace metalcraft {

bool PrepareIndirectCommands(const void* source, std::size_t drawCount, std::size_t stride,
                             const IndirectCommandAllocator& allocator,
                             PreparedIndirectCommands& prepared) {
    prepared = {};
    if (source == nullptr || drawCount == 0) {
        return false;
    }

    const std::size_t sourceStride =
        stride == 0 ? sizeof(IndirectDrawArraysCommand) : stride;
    if (sourceStride < sizeof(IndirectDrawArraysCommand)) {
        return false;
    }

    const auto sourceAddress = reinterpret_cast<std::uintptr_t>(source);
    if (sourceStride == sizeof(IndirectDrawArraysCommand) &&
        (sourceAddress % alignof(IndirectDrawArraysCommand)) == 0U) {
        prepared.commands = static_cast<const IndirectDrawArraysCommand*>(source);
        prepared.commandCount = drawCount;
        prepared.stride = sourceStride;
        prepared.byteLength = drawCount * sizeof(IndirectDrawArraysCommand);
        prepared.zeroCopy = true;
        return true;
    }

    if (!allocator) {
        return false;
    }

    RingAllocation allocation{};
    if (drawCount >
        (std::numeric_limits<std::size_t>::max() / sizeof(IndirectDrawArraysCommand))) {
        return false;
    }
    const std::size_t packedSize = drawCount * sizeof(IndirectDrawArraysCommand);
    if (!allocator(packedSize, alignof(IndirectDrawArraysCommand), allocation) ||
        allocation.cpuAddress == nullptr || allocation.size < packedSize) {
        return false;
    }

    auto* destination =
        reinterpret_cast<IndirectDrawArraysCommand*>(allocation.cpuAddress);
    const auto* sourceBytes = static_cast<const std::uint8_t*>(source);
    for (std::size_t index = 0; index < drawCount; ++index) {
        std::memcpy(destination + index, sourceBytes + index * sourceStride,
                    sizeof(IndirectDrawArraysCommand));
    }

    prepared.commands = destination;
    prepared.commandCount = drawCount;
    prepared.stride = sizeof(IndirectDrawArraysCommand);
    prepared.byteLength = packedSize;
    prepared.ringOffset = allocation.offset;
    prepared.stagedInRing = true;
    return true;
}

} // namespace metalcraft
