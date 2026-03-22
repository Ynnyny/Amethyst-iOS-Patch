#pragma once

#include "metalcraft/RenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace metalcraft {

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual std::uint32_t width() const noexcept = 0;
    virtual std::uint32_t height() const noexcept = 0;
    virtual PixelFormat format() const noexcept = 0;
    virtual bool upload(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h,
                        const void* data, std::size_t bytesPerRow) = 0;
};

using TexturePtr = std::shared_ptr<ITexture>;

} // namespace metalcraft
