#include "metalcraft/JNIBridge.h"
#include "metalcraft/IRenderDevice.h"

#include <mutex>
#include <unordered_map>

namespace metalcraft {
namespace detail {

std::mutex& sharedMutex() {
    static std::mutex mutex;
    return mutex;
}

BlendFactor toBlendFactor(jint value) noexcept {
    switch (value) {
        case static_cast<jint>(BlendFactor::Zero): return BlendFactor::Zero;
        case static_cast<jint>(BlendFactor::One): return BlendFactor::One;
        case static_cast<jint>(BlendFactor::SrcColor): return BlendFactor::SrcColor;
        case static_cast<jint>(BlendFactor::OneMinusSrcColor): return BlendFactor::OneMinusSrcColor;
        case static_cast<jint>(BlendFactor::SrcAlpha): return BlendFactor::SrcAlpha;
        case static_cast<jint>(BlendFactor::OneMinusSrcAlpha): return BlendFactor::OneMinusSrcAlpha;
        case static_cast<jint>(BlendFactor::DstColor): return BlendFactor::DstColor;
        case static_cast<jint>(BlendFactor::OneMinusDstColor): return BlendFactor::OneMinusDstColor;
        case static_cast<jint>(BlendFactor::DstAlpha): return BlendFactor::DstAlpha;
        case static_cast<jint>(BlendFactor::OneMinusDstAlpha): return BlendFactor::OneMinusDstAlpha;
        default: return BlendFactor::One;
    }
}

BlendOp toBlendOp(jint value) noexcept {
    switch (value) {
        case static_cast<jint>(BlendOp::Add): return BlendOp::Add;
        case static_cast<jint>(BlendOp::Subtract): return BlendOp::Subtract;
        case static_cast<jint>(BlendOp::ReverseSubtract): return BlendOp::ReverseSubtract;
        case static_cast<jint>(BlendOp::Min): return BlendOp::Min;
        case static_cast<jint>(BlendOp::Max): return BlendOp::Max;
        default: return BlendOp::Add;
    }
}

CompareOp toCompareOp(jint value) noexcept {
    switch (value) {
        case static_cast<jint>(CompareOp::Never): return CompareOp::Never;
        case static_cast<jint>(CompareOp::Less): return CompareOp::Less;
        case static_cast<jint>(CompareOp::LessEqual): return CompareOp::LessEqual;
        case static_cast<jint>(CompareOp::Equal): return CompareOp::Equal;
        case static_cast<jint>(CompareOp::Greater): return CompareOp::Greater;
        case static_cast<jint>(CompareOp::GreaterEqual): return CompareOp::GreaterEqual;
        case static_cast<jint>(CompareOp::Always): return CompareOp::Always;
        default: return CompareOp::LessEqual;
    }
}

PrimitiveTopology toTopology(jint value) noexcept {
    switch (value) {
        case static_cast<jint>(PrimitiveTopology::Triangles): return PrimitiveTopology::Triangles;
        case static_cast<jint>(PrimitiveTopology::TriangleStrip): return PrimitiveTopology::TriangleStrip;
        case static_cast<jint>(PrimitiveTopology::Lines): return PrimitiveTopology::Lines;
        case static_cast<jint>(PrimitiveTopology::LineStrip): return PrimitiveTopology::LineStrip;
        case static_cast<jint>(PrimitiveTopology::Points): return PrimitiveTopology::Points;
        default: return PrimitiveTopology::Triangles;
    }
}

PixelFormat toPixelFormat(jint value) noexcept {
    switch (value) {
        case static_cast<jint>(PixelFormat::BGRA8Unorm): return PixelFormat::BGRA8Unorm;
        case static_cast<jint>(PixelFormat::RGBA16Float): return PixelFormat::RGBA16Float;
        case static_cast<jint>(PixelFormat::Depth32Float): return PixelFormat::Depth32Float;
        case static_cast<jint>(PixelFormat::Depth24Stencil8): return PixelFormat::Depth24Stencil8;
        default: return PixelFormat::Invalid;
    }
}

StateTracker& trackerStorage() {
    static StateTracker tracker;
    return tracker;
}

std::unique_ptr<IRenderDevice>& deviceStorage() {
    static std::unique_ptr<IRenderDevice> device;
    return device;
}

std::unordered_map<std::uint64_t, TexturePtr>& textureMapStorage() {
    static std::unordered_map<std::uint64_t, TexturePtr> textures;
    return textures;
}

std::uint64_t nextTextureId() {
    static std::uint64_t id = 1;
    return id++;
}

} // namespace detail

StateTracker& GetSharedStateTracker() {
    return detail::trackerStorage();
}

} // namespace metalcraft

extern "C" {

void MetalCraftJNI_ResetStateTracker() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::GetSharedStateTracker().reset();
}

void MetalCraftJNI_SetBlendState(jboolean enabled, jint srcColor, jint dstColor, jint srcAlpha,
                                 jint dstAlpha, jint colorOp, jint alphaOp, jint writeMask) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::StateTracker& tracker = metalcraft::GetSharedStateTracker();
    tracker.setBlendEnabled(enabled != 0);
    tracker.setBlendFactors(metalcraft::detail::toBlendFactor(srcColor),
                            metalcraft::detail::toBlendFactor(dstColor),
                            metalcraft::detail::toBlendFactor(srcAlpha),
                            metalcraft::detail::toBlendFactor(dstAlpha),
                            metalcraft::detail::toBlendOp(colorOp),
                            metalcraft::detail::toBlendOp(alphaOp));
    tracker.setBlendWriteMask(static_cast<std::uint8_t>(writeMask & 0x0F));
}

void MetalCraftJNI_SetDepthState(jboolean testEnabled, jboolean writeEnabled, jint compareOp) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::GetSharedStateTracker().setDepthState(testEnabled != 0, writeEnabled != 0,
                                                      metalcraft::detail::toCompareOp(compareOp));
}

void MetalCraftJNI_BindShaders(jlong vertexShaderId, jlong fragmentShaderId, jlong vertexLayoutId) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::GetSharedStateTracker().bindShaders(static_cast<std::uint64_t>(vertexShaderId),
                                                    static_cast<std::uint64_t>(fragmentShaderId),
                                                    static_cast<std::uint64_t>(vertexLayoutId));
}

void MetalCraftJNI_SetRenderPassState(jint colorFormat, jint depthFormat, jint sampleCount,
                                      jint topology) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::StateTracker& tracker = metalcraft::GetSharedStateTracker();
    tracker.setRenderPassFormats(metalcraft::detail::toPixelFormat(colorFormat),
                                 metalcraft::detail::toPixelFormat(depthFormat),
                                 static_cast<std::uint8_t>(sampleCount));
    tracker.setTopology(metalcraft::detail::toTopology(topology));
}

jlong MetalCraftJNI_CurrentPipelineHash() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    return static_cast<jlong>(metalcraft::GetSharedStateTracker().currentPipelineKey().value);
}

void MetalCraftJNI_BeginFrame() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    auto& device = metalcraft::detail::deviceStorage();
    if (!device) {
        device = metalcraft::CreateRenderDevice(4 * 1024 * 1024); // 4 MB ring buffer
    }
    if (device) {
        device->beginFrame();
    }
}

void MetalCraftJNI_EndFrame() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    auto& device = metalcraft::detail::deviceStorage();
    if (device) {
        device->endFrame();
    }
}

void MetalCraftJNI_Draw(jint topology, jint vertexStart, jint vertexCount, jint instanceCount) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    auto& device = metalcraft::detail::deviceStorage();
    if (device) {
        metalcraft::DrawCallInfo info{};
        info.topology = metalcraft::detail::toTopology(topology);
        info.vertexStart = static_cast<std::uint32_t>(vertexStart);
        info.vertexCount = static_cast<std::uint32_t>(vertexCount);
        info.instanceCount = static_cast<std::uint32_t>(instanceCount);
        device->draw(info);
    }
}

void MetalCraftJNI_DrawIndexed(jint topology, jint indexCount, jlong indexBufferOffset,
                               jint instanceCount) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    auto& device = metalcraft::detail::deviceStorage();
    if (device) {
        metalcraft::DrawCallInfo info{};
        info.topology = metalcraft::detail::toTopology(topology);
        info.indexCount = static_cast<std::uint32_t>(indexCount);
        info.indexBufferOffset = static_cast<std::size_t>(indexBufferOffset);
        info.instanceCount = static_cast<std::uint32_t>(instanceCount);
        device->drawIndexed(info);
    }
}

jlong MetalCraftJNI_CreateTexture(jint width, jint height, jint format, jboolean mipmapped) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    auto& device = metalcraft::detail::deviceStorage();
    if (!device) {
        return 0;
    }

    metalcraft::TextureDescriptor desc{};
    desc.width = static_cast<std::uint32_t>(width);
    desc.height = static_cast<std::uint32_t>(height);
    desc.format = metalcraft::detail::toPixelFormat(format);
    desc.mipmapped = mipmapped != 0;

    auto texture = device->createTexture(desc);
    if (!texture) {
        return 0;
    }

    const std::uint64_t id = metalcraft::detail::nextTextureId();
    metalcraft::detail::textureMapStorage()[id] = std::move(texture);
    return static_cast<jlong>(id);
}

jboolean MetalCraftJNI_UploadTexture(jlong textureId, jint x, jint y, jint w, jint h,
                                     jlong dataPtr, jlong bytesPerRow) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    auto& textures = metalcraft::detail::textureMapStorage();
    auto it = textures.find(static_cast<std::uint64_t>(textureId));
    if (it == textures.end() || !it->second) {
        return JNI_FALSE;
    }

    const void* data = reinterpret_cast<const void*>(dataPtr);
    const bool result = it->second->upload(
        static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y),
        static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h),
        data, static_cast<std::size_t>(bytesPerRow));
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nResetStateTracker(
    JNIEnv*, jclass) {
    MetalCraftJNI_ResetStateTracker();
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nSetBlendState(
    JNIEnv*, jclass, jboolean enabled, jint srcColor, jint dstColor, jint srcAlpha, jint dstAlpha,
    jint colorOp, jint alphaOp, jint writeMask) {
    MetalCraftJNI_SetBlendState(enabled, srcColor, dstColor, srcAlpha, dstAlpha, colorOp, alphaOp,
                                writeMask);
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nSetDepthState(
    JNIEnv*, jclass, jboolean testEnabled, jboolean writeEnabled, jint compareOp) {
    MetalCraftJNI_SetDepthState(testEnabled, writeEnabled, compareOp);
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nBindShaders(
    JNIEnv*, jclass, jlong vertexShaderId, jlong fragmentShaderId, jlong vertexLayoutId) {
    MetalCraftJNI_BindShaders(vertexShaderId, fragmentShaderId, vertexLayoutId);
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nSetRenderPassState(
    JNIEnv*, jclass, jint colorFormat, jint depthFormat, jint sampleCount, jint topology) {
    MetalCraftJNI_SetRenderPassState(colorFormat, depthFormat, sampleCount, topology);
}

JNIEXPORT jlong JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nCurrentPipelineHash(
    JNIEnv*, jclass) {
    return MetalCraftJNI_CurrentPipelineHash();
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nBeginFrame(
    JNIEnv*, jclass) {
    MetalCraftJNI_BeginFrame();
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nEndFrame(
    JNIEnv*, jclass) {
    MetalCraftJNI_EndFrame();
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nDraw(
    JNIEnv*, jclass, jint topology, jint vertexStart, jint vertexCount, jint instanceCount) {
    MetalCraftJNI_Draw(topology, vertexStart, vertexCount, instanceCount);
}

JNIEXPORT void JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nDrawIndexed(
    JNIEnv*, jclass, jint topology, jint indexCount, jlong indexBufferOffset, jint instanceCount) {
    MetalCraftJNI_DrawIndexed(topology, indexCount, indexBufferOffset, instanceCount);
}

JNIEXPORT jlong JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nCreateTexture(
    JNIEnv*, jclass, jint width, jint height, jint format, jboolean mipmapped) {
    return MetalCraftJNI_CreateTexture(width, height, format, mipmapped);
}

JNIEXPORT jboolean JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nUploadTexture(
    JNIEnv*, jclass, jlong textureId, jint x, jint y, jint w, jint h, jlong dataPtr,
    jlong bytesPerRow) {
    return MetalCraftJNI_UploadTexture(textureId, x, y, w, h, dataPtr, bytesPerRow);
}

} // extern "C"
