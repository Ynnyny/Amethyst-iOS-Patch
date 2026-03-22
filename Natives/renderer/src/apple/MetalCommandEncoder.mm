#ifdef __APPLE__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "metalcraft/MetalCommandEncoder.h"

namespace metalcraft {
namespace {

class CommandEncoderBox {
public:
    explicit CommandEncoderBox(const void* commandQueueHandle)
        : commandQueueHandle_(commandQueueHandle) {}

    ~CommandEncoderBox() = default;

    void beginFrame() {
        @autoreleasepool {
            if (commandQueueHandle_ == nullptr) {
                return;
            }

            id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)commandQueueHandle_;
            id<MTLCommandBuffer> cmdBuffer = [queue commandBuffer];
            if (cmdBuffer != nil) {
                currentCommandBufferHandle_ = CFBridgingRetain(cmdBuffer);
            }
        }
    }

    void endFrame() {
        @autoreleasepool {
            if (currentCommandBufferHandle_ != nullptr) {
                id<MTLCommandBuffer> cmdBuffer =
                    (__bridge id<MTLCommandBuffer>)currentCommandBufferHandle_;
                [cmdBuffer commit];
                (void)CFBridgingRelease(currentCommandBufferHandle_);
                currentCommandBufferHandle_ = nullptr;
            }
        }
    }

private:
    const void* commandQueueHandle_ = nullptr;
    const void* currentCommandBufferHandle_ = nullptr;
};

} // namespace

void* MetalCommandEncoderCreate(const void* commandQueueHandle) {
    return new CommandEncoderBox(commandQueueHandle);
}

void MetalCommandEncoderBeginFrame(void* encoderHandle) {
    if (encoderHandle != nullptr) {
        static_cast<CommandEncoderBox*>(encoderHandle)->beginFrame();
    }
}

void MetalCommandEncoderEndFrame(void* encoderHandle) {
    if (encoderHandle != nullptr) {
        static_cast<CommandEncoderBox*>(encoderHandle)->endFrame();
    }
}

void MetalCommandEncoderDestroy(void* encoderHandle) {
    delete static_cast<CommandEncoderBox*>(encoderHandle);
}

} // namespace metalcraft

#endif
