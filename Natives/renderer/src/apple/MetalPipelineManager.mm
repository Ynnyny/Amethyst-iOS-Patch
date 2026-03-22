#ifdef __APPLE__

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "metalcraft/RenderTypes.h"

namespace metalcraft {
namespace {

MTLPixelFormat toPixelFormat(PixelFormat format) {
    switch (format) {
        case PixelFormat::BGRA8Unorm: return MTLPixelFormatBGRA8Unorm;
        case PixelFormat::RGBA16Float: return MTLPixelFormatRGBA16Float;
        case PixelFormat::Depth32Float: return MTLPixelFormatDepth32Float;
        case PixelFormat::Depth24Stencil8: return MTLPixelFormatDepth32Float_Stencil8;
        case PixelFormat::Invalid:
        default: return MTLPixelFormatInvalid;
    }
}

MTLBlendFactor toBlendFactor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero: return MTLBlendFactorZero;
        case BlendFactor::One: return MTLBlendFactorOne;
        case BlendFactor::SrcColor: return MTLBlendFactorSourceColor;
        case BlendFactor::OneMinusSrcColor: return MTLBlendFactorOneMinusSourceColor;
        case BlendFactor::SrcAlpha: return MTLBlendFactorSourceAlpha;
        case BlendFactor::OneMinusSrcAlpha: return MTLBlendFactorOneMinusSourceAlpha;
        case BlendFactor::DstColor: return MTLBlendFactorDestinationColor;
        case BlendFactor::OneMinusDstColor: return MTLBlendFactorOneMinusDestinationColor;
        case BlendFactor::DstAlpha: return MTLBlendFactorDestinationAlpha;
        case BlendFactor::OneMinusDstAlpha: return MTLBlendFactorOneMinusDestinationAlpha;
        default: return MTLBlendFactorOne;
    }
}

MTLBlendOperation toBlendOperation(BlendOp operation) {
    switch (operation) {
        case BlendOp::Add: return MTLBlendOperationAdd;
        case BlendOp::Subtract: return MTLBlendOperationSubtract;
        case BlendOp::ReverseSubtract: return MTLBlendOperationReverseSubtract;
        case BlendOp::Min: return MTLBlendOperationMin;
        case BlendOp::Max: return MTLBlendOperationMax;
        default: return MTLBlendOperationAdd;
    }
}

MTLColorWriteMask toColorWriteMask(std::uint8_t mask) {
    MTLColorWriteMask writeMask = static_cast<MTLColorWriteMask>(0);
    if ((mask & 0x1U) != 0U) {
        writeMask |= MTLColorWriteMaskRed;
    }
    if ((mask & 0x2U) != 0U) {
        writeMask |= MTLColorWriteMaskGreen;
    }
    if ((mask & 0x4U) != 0U) {
        writeMask |= MTLColorWriteMaskBlue;
    }
    if ((mask & 0x8U) != 0U) {
        writeMask |= MTLColorWriteMaskAlpha;
    }
    return writeMask;
}

id<MTLRenderPipelineState> buildPipeline(const void* deviceHandle, const PipelineDescriptor& descriptor) {
    if (deviceHandle == nullptr || descriptor.vertexMSL.empty() || descriptor.fragmentMSL.empty()) {
        return nil;
    }

    id<MTLDevice> device = (__bridge id<MTLDevice>)deviceHandle;
    NSString* vertexSource = [NSString stringWithUTF8String:descriptor.vertexMSL.c_str()];
    NSString* fragmentSource = [NSString stringWithUTF8String:descriptor.fragmentMSL.c_str()];
    NSString* combinedSource = [NSString stringWithFormat:@"%@\n%@", vertexSource, fragmentSource];

    NSError* libraryError = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:combinedSource options:nil error:&libraryError];
    if (library == nil || libraryError != nil) {
        return nil;
    }

    NSString* vertexFunctionName = [NSString stringWithUTF8String:descriptor.vertexFunction.c_str()];
    NSString* fragmentFunctionName = [NSString stringWithUTF8String:descriptor.fragmentFunction.c_str()];
    id<MTLFunction> vertexFunction = [library newFunctionWithName:vertexFunctionName];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:fragmentFunctionName];
    if (vertexFunction == nil || fragmentFunction == nil) {
        return nil;
    }

    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.sampleCount = descriptor.snapshot.formats.sampleCount == 0
                                         ? 1
                                         : descriptor.snapshot.formats.sampleCount;
    pipelineDescriptor.colorAttachments[0].pixelFormat =
        toPixelFormat(descriptor.snapshot.formats.colorFormat);
    pipelineDescriptor.depthAttachmentPixelFormat =
        toPixelFormat(descriptor.snapshot.formats.depthFormat);
    pipelineDescriptor.stencilAttachmentPixelFormat =
        descriptor.snapshot.formats.depthFormat == PixelFormat::Depth24Stencil8
            ? MTLPixelFormatDepth32Float_Stencil8
            : MTLPixelFormatInvalid;

    const BlendState& blend = descriptor.snapshot.blend;
    MTLRenderPipelineColorAttachmentDescriptor* attachment = pipelineDescriptor.colorAttachments[0];
    attachment.blendingEnabled = blend.enabled;
    attachment.sourceRGBBlendFactor = toBlendFactor(blend.srcColor);
    attachment.destinationRGBBlendFactor = toBlendFactor(blend.dstColor);
    attachment.rgbBlendOperation = toBlendOperation(blend.colorOp);
    attachment.sourceAlphaBlendFactor = toBlendFactor(blend.srcAlpha);
    attachment.destinationAlphaBlendFactor = toBlendFactor(blend.dstAlpha);
    attachment.alphaBlendOperation = toBlendOperation(blend.alphaOp);
    attachment.writeMask = toColorWriteMask(blend.writeMask);

    NSError* pipelineError = nil;
    return [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&pipelineError];
}

class PipelineManagerBox {
public:
    explicit PipelineManagerBox(const void* deviceHandle)
        : deviceHandle_(deviceHandle),
          cacheHandle_(CFBridgingRetain([[NSMutableDictionary alloc] init])) {}

    ~PipelineManagerBox() {
        if (cacheHandle_ != nullptr) {
            (void)CFBridgingRelease(cacheHandle_);
            cacheHandle_ = nullptr;
        }
    }

    std::uint64_t acquire(const PipelineDescriptor& descriptor) {
        NSMutableDictionary* cache = (__bridge NSMutableDictionary*)cacheHandle_;
        NSNumber* cacheKey = @(descriptor.key.value);
        id<MTLRenderPipelineState> pipeline = cache[cacheKey];
        if (pipeline == nil) {
            pipeline = buildPipeline(deviceHandle_, descriptor);
            if (pipeline == nil) {
                return 0;
            }
            cache[cacheKey] = pipeline;
        }
        return descriptor.key.value;
    }

private:
    const void* deviceHandle_ = nullptr;
    const void* cacheHandle_ = nullptr;
};

} // namespace

void* MetalPipelineManagerCreate(const void* deviceHandle) {
    return new PipelineManagerBox(deviceHandle);
}

std::uint64_t MetalPipelineManagerAcquire(void* managerHandle, const PipelineDescriptor& descriptor) {
    if (managerHandle == nullptr) {
        return 0;
    }
    return static_cast<PipelineManagerBox*>(managerHandle)->acquire(descriptor);
}

void MetalPipelineManagerDestroy(void* managerHandle) {
    delete static_cast<PipelineManagerBox*>(managerHandle);
}

} // namespace metalcraft

#endif
