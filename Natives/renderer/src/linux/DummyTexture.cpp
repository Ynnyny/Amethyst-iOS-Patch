#include "DummyBackend.h"

#ifdef __linux__

#include <cstring>
#include <vector>

namespace metalcraft {
namespace {

class DummyTexture final : public ITexture {
public:
    explicit DummyTexture(const TextureDescriptor& descriptor)
        : descriptor_(descriptor),
          storage_(static_cast<std::size_t>(descriptor.width) * descriptor.height * 4, 0) {}

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
        if (data == nullptr || x + w > descriptor_.width || y + h > descriptor_.height) {
            return false;
        }

        const std::size_t dstRowBytes = static_cast<std::size_t>(descriptor_.width) * 4;
        const auto* src = static_cast<const std::uint8_t*>(data);
        for (std::uint32_t row = 0; row < h; ++row) {
            const std::size_t dstOffset = (static_cast<std::size_t>(y + row) * dstRowBytes) +
                                          (static_cast<std::size_t>(x) * 4);
            const std::size_t copyLen = static_cast<std::size_t>(w) * 4;
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
    return std::make_shared<DummyTexture>(descriptor);
}

} // namespace metalcraft

#endif
