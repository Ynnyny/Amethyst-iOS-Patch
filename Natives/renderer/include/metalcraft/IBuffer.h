#pragma once

#include "metalcraft/RenderTypes.h"

#include <cstddef>
#include <memory>

namespace metalcraft {

class IBuffer {
public:
    virtual ~IBuffer() = default;

    virtual std::size_t size() const noexcept = 0;
    virtual BufferUsage usage() const noexcept = 0;
    virtual bool update(std::size_t offset, const void* data, std::size_t length) = 0;
    virtual void* mappedData() noexcept = 0;
};

using BufferPtr = std::shared_ptr<IBuffer>;

} // namespace metalcraft
