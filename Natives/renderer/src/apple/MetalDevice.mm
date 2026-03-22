#ifdef __APPLE__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "metalcraft/IRenderDevice.h"
#include "metalcraft/MetalCommandEncoder.h"

#include <algorithm>

namespace metalcraft {

BufferPtr CreateMetalBuffer(const void* deviceHandle, const BufferDescriptor& descriptor);
TexturePtr CreateMetalTexture(const void* deviceHandle, const TextureDescriptor& descriptor);
void* MetalPipelineManagerCreate(const void* deviceHandle);
std::uint64_t MetalPipelineManagerAcquire(void* managerHandle, const PipelineDescriptor& descriptor);
void MetalPipelineManagerDestroy(void* managerHandle);
void* MetalDepthStencilManagerCreate(const void* deviceHandle);
std::uint64_t MetalDepthStencilManagerAcquire(void* managerHandle, const DepthState& state);
void MetalDepthStencilManagerDestroy(void* managerHandle);

namespace {

std::size_t alignUp(std::size_t value, std::size_t alignment) noexcept {
    const std::size_t safeAlignment = std::max<std::size_t>(alignment, 1U);
    const std::size_t remainder = value % safeAlignment;
    return remainder == 0 ? value : (value + safeAlignment - remainder);
}

class MetalRenderDevice final : public IRenderDevice {
public:
    explicit MetalRenderDevice(std::size_t ringBufferCapacity)
        : ringBufferCapacity_(std::max<std::size_t>(ringBufferCapacity, 1U)) {
        @autoreleasepool {
            id<MTLDevice> device = MTLCreateSystemDefaultDevice();
            if (device == nil) {
                return;
            }

            deviceHandle_ = CFBridgingRetain(device);

            id<MTLCommandQueue> queue = [device newCommandQueue];
            if (queue != nil) {
                commandQueueHandle_ = CFBridgingRetain(queue);
            }

            id<MTLBuffer> uploadBuffer =
                [device newBufferWithLength:ringBufferCapacity_ options:MTLResourceStorageModeShared];
            if (uploadBuffer != nil) {
                uploadBufferHandle_ = CFBridgingRetain(uploadBuffer);
            }

            pipelineManagerHandle_ = MetalPipelineManagerCreate(deviceHandle_);
            depthStencilManagerHandle_ = MetalDepthStencilManagerCreate(deviceHandle_);
            commandEncoderHandle_ = MetalCommandEncoderCreate(commandQueueHandle_);
        }
    }

    ~MetalRenderDevice() override {
        MetalCommandEncoderDestroy(commandEncoderHandle_);
        commandEncoderHandle_ = nullptr;

        MetalDepthStencilManagerDestroy(depthStencilManagerHandle_);
        depthStencilManagerHandle_ = nullptr;

        MetalPipelineManagerDestroy(pipelineManagerHandle_);
        pipelineManagerHandle_ = nullptr;

        if (uploadBufferHandle_ != nullptr) {
            (void)CFBridgingRelease(uploadBufferHandle_);
            uploadBufferHandle_ = nullptr;
        }
        if (commandQueueHandle_ != nullptr) {
            (void)CFBridgingRelease(commandQueueHandle_);
            commandQueueHandle_ = nullptr;
        }
        if (deviceHandle_ != nullptr) {
            (void)CFBridgingRelease(deviceHandle_);
            deviceHandle_ = nullptr;
        }
    }

    BufferPtr createBuffer(const BufferDescriptor& descriptor) override {
        return CreateMetalBuffer(deviceHandle_, descriptor);
    }

    TexturePtr createTexture(const TextureDescriptor& descriptor) override {
        return CreateMetalTexture(deviceHandle_, descriptor);
    }

    std::uint64_t acquirePipeline(const PipelineDescriptor& descriptor) override {
        if (pipelineManagerHandle_ == nullptr) {
            return 0;
        }
        return MetalPipelineManagerAcquire(pipelineManagerHandle_, descriptor);
    }

    std::uint64_t acquireDepthStencilState(const DepthState& state) override {
        if (depthStencilManagerHandle_ == nullptr) {
            return 0;
        }
        return MetalDepthStencilManagerAcquire(depthStencilManagerHandle_, state);
    }

    bool writeRing(std::size_t size, std::size_t alignment, RingAllocation& allocation) override {
        if (uploadBufferHandle_ == nullptr || size == 0 || size > ringBufferCapacity_) {
            return false;
        }

        std::size_t alignedOffset = alignUp(ringOffset_, alignment);
        if (alignedOffset + size > ringBufferCapacity_) {
            alignedOffset = 0;
        }
        if (alignedOffset + size > ringBufferCapacity_) {
            return false;
        }

        id<MTLBuffer> uploadBuffer = (__bridge id<MTLBuffer>)uploadBufferHandle_;
        std::uint8_t* baseAddress = static_cast<std::uint8_t*>([uploadBuffer contents]);
        if (baseAddress == nullptr) {
            return false;
        }

        allocation.offset = alignedOffset;
        allocation.size = size;
        allocation.cpuAddress = baseAddress + alignedOffset;
        ringOffset_ = alignedOffset + size;
        return true;
    }

    void beginFrame() override {
        ringOffset_ = 0;
        drawCallCount_ = 0;
        MetalCommandEncoderBeginFrame(commandEncoderHandle_);
    }

    void endFrame() override {
        MetalCommandEncoderEndFrame(commandEncoderHandle_);
    }

    void draw(const DrawCallInfo& info) override {
        (void)info;
        ++drawCallCount_;
    }

    void drawIndexed(const DrawCallInfo& info) override {
        (void)info;
        ++drawCallCount_;
    }

    void submit() override {
        // Command encoder handles commit in endFrame
    }

    std::size_t ringBufferCapacity() const noexcept override {
        return ringBufferCapacity_;
    }

    std::size_t drawCallCount() const noexcept override {
        return drawCallCount_;
    }

private:
    const void* deviceHandle_ = nullptr;
    const void* commandQueueHandle_ = nullptr;
    const void* uploadBufferHandle_ = nullptr;
    void* pipelineManagerHandle_ = nullptr;
    void* depthStencilManagerHandle_ = nullptr;
    void* commandEncoderHandle_ = nullptr;
    std::size_t ringBufferCapacity_ = 0;
    std::size_t ringOffset_ = 0;
    std::size_t drawCallCount_ = 0;
};

} // namespace

std::unique_ptr<IRenderDevice> CreateMetalRenderDevice(std::size_t ringBufferCapacity) {
    return std::make_unique<MetalRenderDevice>(ringBufferCapacity);
}

} // namespace metalcraft

#endif
