#ifdef __APPLE__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "metalcraft/ITexture.h"

#include <cstring>

namespace metalcraft {
namespace {

MTLPixelFormat toMTLPixelFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::BGRA8Unorm: return MTLPixelFormatBGRA8Unorm;
        case PixelFormat::RGBA16Float: return MTLPixelFormatRGBA16Float;
        case PixelFormat::Depth32Float: return MTLPixelFormatDepth32Float;
        case PixelFormat::Depth24Stencil8: return MTLPixelFormatDepth32Float_Stencil8;
        case PixelFormat::Invalid:
        default: return MTLPixelFormatInvalid;
    }
}

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

std::uint32_t computeMipLevelCount(const TextureDescriptor& descriptor) noexcept {
    if (!descriptor.mipmapped) {
        return 1;
    }

    std::uint32_t width = descriptor.width;
    std::uint32_t height = descriptor.height;
    std::uint32_t levels = 1;
    while (width > 1 || height > 1) {
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
        ++levels;
    }
    return levels;
}

class MetalTexture final : public ITexture {
public:
    MetalTexture(const void* textureHandle, const TextureDescriptor& descriptor)
        : textureHandle_(textureHandle), descriptor_(descriptor) {}

    ~MetalTexture() override {
        if (textureHandle_ != nullptr) {
            (void)CFBridgingRelease(textureHandle_);
            textureHandle_ = nullptr;
        }
    }

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
        if (data == nullptr || textureHandle_ == nullptr || w == 0 || h == 0 ||
            x + w > descriptor_.width || y + h > descriptor_.height ||
            pixelStride == 0 || bytesPerRow < static_cast<std::size_t>(w) * pixelStride) {
            return false;
        }

        id<MTLTexture> texture = (__bridge id<MTLTexture>)textureHandle_;
        MTLRegion region = MTLRegionMake2D(x, y, w, h);
        [texture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];
        return true;
    }

private:
    const void* textureHandle_ = nullptr;
    TextureDescriptor descriptor_{};
};

} // namespace

TexturePtr CreateMetalTexture(const void* deviceHandle, const TextureDescriptor& descriptor) {
    @autoreleasepool {
        if (deviceHandle == nullptr || descriptor.width == 0 || descriptor.height == 0 ||
            descriptor.format == PixelFormat::Invalid) {
            return {};
        }

        id<MTLDevice> device = (__bridge id<MTLDevice>)deviceHandle;
        MTLTextureDescriptor* texDesc = [[MTLTextureDescriptor alloc] init];
        texDesc.textureType = MTLTextureType2D;
        texDesc.pixelFormat = toMTLPixelFormat(descriptor.format);
        if (texDesc.pixelFormat == MTLPixelFormatInvalid) {
            return {};
        }
        texDesc.width = descriptor.width;
        texDesc.height = descriptor.height;
        texDesc.mipmapLevelCount = computeMipLevelCount(descriptor);
        texDesc.storageMode = MTLStorageModeShared;
        texDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite |
                        MTLTextureUsageRenderTarget;

        id<MTLTexture> texture = [device newTextureWithDescriptor:texDesc];
        if (texture == nil) {
            return {};
        }

        return std::make_shared<MetalTexture>(CFBridgingRetain(texture), descriptor);
    }
}

} // namespace metalcraft

#endif
