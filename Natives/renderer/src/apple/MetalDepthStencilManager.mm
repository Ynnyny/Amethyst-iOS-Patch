#ifdef __APPLE__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "metalcraft/RenderTypes.h"

namespace metalcraft {
namespace {

MTLCompareFunction toCompareFunction(CompareOp op) {
    switch (op) {
        case CompareOp::Never: return MTLCompareFunctionNever;
        case CompareOp::Less: return MTLCompareFunctionLess;
        case CompareOp::LessEqual: return MTLCompareFunctionLessEqual;
        case CompareOp::Equal: return MTLCompareFunctionEqual;
        case CompareOp::Greater: return MTLCompareFunctionGreater;
        case CompareOp::GreaterEqual: return MTLCompareFunctionGreaterEqual;
        case CompareOp::Always: return MTLCompareFunctionAlways;
        default: return MTLCompareFunctionLessEqual;
    }
}

std::uint64_t hashDepthState(const DepthState& state) noexcept {
    std::uint64_t h = 0;
    h |= (state.testEnabled ? 1ULL : 0ULL);
    h |= (state.writeEnabled ? 2ULL : 0ULL);
    h |= (static_cast<std::uint64_t>(state.compareOp) << 2U);
    return h == 0 ? 1 : h;
}

class DepthStencilManagerBox {
public:
    explicit DepthStencilManagerBox(const void* deviceHandle)
        : deviceHandle_(deviceHandle),
          cacheHandle_(CFBridgingRetain([[NSMutableDictionary alloc] init])) {}

    ~DepthStencilManagerBox() {
        if (cacheHandle_ != nullptr) {
            (void)CFBridgingRelease(cacheHandle_);
            cacheHandle_ = nullptr;
        }
    }

    std::uint64_t acquire(const DepthState& state) {
        @autoreleasepool {
            const std::uint64_t key = hashDepthState(state);
            NSMutableDictionary* cache = (__bridge NSMutableDictionary*)cacheHandle_;
            NSNumber* cacheKey = @(key);
            id<MTLDepthStencilState> depthStencilState = cache[cacheKey];

            if (depthStencilState == nil) {
                id<MTLDevice> device = (__bridge id<MTLDevice>)deviceHandle_;
                MTLDepthStencilDescriptor* desc = [[MTLDepthStencilDescriptor alloc] init];
                desc.depthCompareFunction = state.testEnabled
                    ? toCompareFunction(state.compareOp)
                    : MTLCompareFunctionAlways;
                desc.depthWriteEnabled = state.writeEnabled;

                depthStencilState = [device newDepthStencilStateWithDescriptor:desc];
                if (depthStencilState == nil) {
                    return 0;
                }
                cache[cacheKey] = depthStencilState;
            }

            return key;
        }
    }

private:
    const void* deviceHandle_ = nullptr;
    const void* cacheHandle_ = nullptr;
};

} // namespace

void* MetalDepthStencilManagerCreate(const void* deviceHandle) {
    return new DepthStencilManagerBox(deviceHandle);
}

std::uint64_t MetalDepthStencilManagerAcquire(void* managerHandle, const DepthState& state) {
    if (managerHandle == nullptr) {
        return 0;
    }
    return static_cast<DepthStencilManagerBox*>(managerHandle)->acquire(state);
}

void MetalDepthStencilManagerDestroy(void* managerHandle) {
    delete static_cast<DepthStencilManagerBox*>(managerHandle);
}

} // namespace metalcraft

#endif
