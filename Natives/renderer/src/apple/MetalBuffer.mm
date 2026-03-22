#ifdef __APPLE__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "metalcraft/IBuffer.h"

#include <cstring>

namespace metalcraft {
namespace {

class MetalBuffer final : public IBuffer {
public:
    MetalBuffer(const void* bufferHandle, const BufferDescriptor& descriptor)
        : bufferHandle_(bufferHandle),
          descriptor_(descriptor) {}

    ~MetalBuffer() override {
        if (bufferHandle_ != nullptr) {
            (void)CFBridgingRelease(bufferHandle_);
            bufferHandle_ = nullptr;
        }
    }

    std::size_t size() const noexcept override {
        return descriptor_.size;
    }

    BufferUsage usage() const noexcept override {
        return descriptor_.usage;
    }

    bool update(std::size_t offset, const void* data, std::size_t length) override {
        if (data == nullptr || offset > descriptor_.size || length > descriptor_.size - offset) {
            return false;
        }

        id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)bufferHandle_;
        void* contents = [buffer contents];
        if (contents == nullptr) {
            return false;
        }

        std::memcpy(static_cast<std::uint8_t*>(contents) + offset, data, length);
        return true;
    }

    void* mappedData() noexcept override {
        id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)bufferHandle_;
        return [buffer contents];
    }

private:
    const void* bufferHandle_ = nullptr;
    BufferDescriptor descriptor_{};
};

MTLResourceOptions bufferOptionsForDescriptor(const BufferDescriptor& descriptor) {
    if (descriptor.cpuVisible || descriptor.usage == BufferUsage::Upload ||
        descriptor.usage == BufferUsage::Uniform) {
        return MTLResourceStorageModeShared;
    }
    return MTLResourceStorageModePrivate;
}

} // namespace

BufferPtr CreateMetalBuffer(const void* deviceHandle, const BufferDescriptor& descriptor) {
    @autoreleasepool {
        if (deviceHandle == nullptr || descriptor.size == 0) {
            return {};
        }

        id<MTLDevice> device = (__bridge id<MTLDevice>)deviceHandle;
        id<MTLBuffer> buffer =
            [device newBufferWithLength:descriptor.size options:bufferOptionsForDescriptor(descriptor)];
        if (buffer == nil) {
            return {};
        }

        return std::make_shared<MetalBuffer>(CFBridgingRetain(buffer), descriptor);
    }
}

} // namespace metalcraft

#endif
