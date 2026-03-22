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
        if (data == nullptr || textureHandle_ == nullptr) {
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
        if (deviceHandle == nullptr || descriptor.width == 0 || descriptor.height == 0) {
            return {};
        }

        id<MTLDevice> device = (__bridge id<MTLDevice>)deviceHandle;
        MTLTextureDescriptor* texDesc = [[MTLTextureDescriptor alloc] init];
        texDesc.textureType = MTLTextureType2D;
        texDesc.pixelFormat = toMTLPixelFormat(descriptor.format);
        texDesc.width = descriptor.width;
        texDesc.height = descriptor.height;
        texDesc.mipmapLevelCount = descriptor.mipmapped ? 1 : 1; // TODO: calculate mip levels
        texDesc.storageMode = MTLStorageModeShared;
        texDesc.usage = MTLTextureUsageShaderRead;

        id<MTLTexture> texture = [device newTextureWithDescriptor:texDesc];
        if (texture == nil) {
            return {};
        }

        return std::make_shared<MetalTexture>(CFBridgingRetain(texture), descriptor);
    }
}

} // namespace metalcraft

#endif
