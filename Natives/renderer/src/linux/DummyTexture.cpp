#include "DummyBackend.h"

#ifdef __linux__

#include <cstring>
#include <vector>

namespace metalcraft {
namespace {

std::size_t bytesPerPixel(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::BGRA8Unorm: return 4;
        case PixelFormat::RGBA16Float: return 8;
        case PixelFormat::Depth32Float: return 4;
        case PixelFormat::Depth24Stencil8: return 4;
        case PixelFormat::Invalid:
        default: return 0;
    }
}

class DummyTexture final : public ITexture {
public:
    explicit DummyTexture(const TextureDescriptor& descriptor)
        : descriptor_(descriptor),
          storage_(static_cast<std::size_t>(descriptor.width) * descriptor.height *
                       bytesPerPixel(descriptor.format),
                   0) {}

    std::uint32_t width() const noexcept override {
        return descriptor_.width;
    }

    std::uint32_t height() const noexcept override {
        return descriptor_.height;
    }

    PixelFormat format() const noexcept override {
        return descriptor_.format;
    }

    bool upload(std::uint32_t x, std::uint32_t y, std::uint32_t w, std::uint32_t h,
                const void* data, std::size_t bytesPerRow) override {
        const std::size_t pixelStride = bytesPerPixel(descriptor_.format);
        if (data == nullptr || descriptor_.format == PixelFormat::Invalid || w == 0 || h == 0 ||
            x + w > descriptor_.width || y + h > descriptor_.height ||
            pixelStride == 0 || bytesPerRow < static_cast<std::size_t>(w) * pixelStride) {
            return false;
        }

        const std::size_t dstRowBytes = static_cast<std::size_t>(descriptor_.width) * pixelStride;
        const auto* src = static_cast<const std::uint8_t*>(data);
        for (std::uint32_t row = 0; row < h; ++row) {
            const std::size_t dstOffset = (static_cast<std::size_t>(y + row) * dstRowBytes) +
                                          (static_cast<std::size_t>(x) * pixelStride);
            const std::size_t copyLen = static_cast<std::size_t>(w) * pixelStride;
            if (dstOffset + copyLen <= storage_.size()) {
                std::memcpy(storage_.data() + dstOffset, src + row * bytesPerRow, copyLen);
            }
        }
        return true;
    }

private:
    TextureDescriptor descriptor_{};
    std::vector<std::uint8_t> storage_;
};

} // namespace

TexturePtr CreateDummyTexture(const TextureDescriptor& descriptor) {
    if (descriptor.width == 0 || descriptor.height == 0 ||
        descriptor.format == PixelFormat::Invalid) {
        return {};
    }
    return std::make_shared<DummyTexture>(descriptor);
}

} // namespace metalcraft

#endif
