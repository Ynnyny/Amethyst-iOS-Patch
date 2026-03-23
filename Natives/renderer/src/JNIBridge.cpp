#include "metalcraft/JNIBridge.h"
#include "metalcraft/IRenderDevice.h"
#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
#include "metalcraft/ShaderTranslator.h"
#endif

#include <cctype>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace metalcraft {
namespace detail {

struct RegisteredShader {
#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
    ShaderStage stage = ShaderStage::Vertex;
    ShaderTranslationOptions options{};
    ShaderTranslationResult translation{};
#else
    jint stage = 0;
    bool flipVertexY = false;
#endif
    std::string glslSource{};
    bool translated = false;
};

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

#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
ShaderStage toShaderStage(jint value) noexcept {
    switch (value) {
        case static_cast<jint>(ShaderStage::Vertex): return ShaderStage::Vertex;
        case static_cast<jint>(ShaderStage::Fragment): return ShaderStage::Fragment;
        case static_cast<jint>(ShaderStage::Compute): return ShaderStage::Compute;
        default: return ShaderStage::Vertex;
    }
}
#endif

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

std::unordered_map<std::uint64_t, RegisteredShader>& shaderMapStorage() {
    static std::unordered_map<std::uint64_t, RegisteredShader> shaders;
    return shaders;
}

std::uint64_t nextTextureId() {
    static std::uint64_t id = 1;
    return id++;
}

IRenderDevice* ensureDevice() {
    auto& device = deviceStorage();
    if (!device) {
        device = metalcraft::CreateRenderDevice(4 * 1024 * 1024);
    }
    return device.get();
}

#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
ShaderTranslator& shaderTranslator() {
    static ShaderTranslator translator;
    return translator;
}

std::string trim(std::string_view value) {
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return std::string(value.substr(begin, end - begin));
}

std::string inferStageFunctionName(const std::string& mslSource, ShaderStage stage,
                                   const std::string& fallback) {
    const char* keyword = stage == ShaderStage::Vertex ? "vertex" : "fragment";
    std::size_t searchStart = 0;
    while (true) {
        const std::size_t keywordPos = mslSource.find(keyword, searchStart);
        if (keywordPos == std::string::npos) {
            return fallback;
        }

        const std::size_t parenPos = mslSource.find('(', keywordPos);
        if (parenPos == std::string::npos) {
            return fallback;
        }

        const std::size_t signatureStart = mslSource.rfind('\n', parenPos);
        const std::size_t lineStart =
            signatureStart == std::string::npos ? 0 : signatureStart + 1;
        const std::string signature = trim(
            std::string_view(mslSource).substr(lineStart, parenPos - lineStart));
        const std::size_t lastSpace = signature.find_last_of(" \t");
        if (lastSpace == std::string::npos || lastSpace + 1 >= signature.size()) {
            searchStart = parenPos + 1;
            continue;
        }

        return signature.substr(lastSpace + 1);
    }
}

bool translateShader(RegisteredShader& shader) {
    if (shader.translated) {
        return shader.translation.succeeded;
    }

    shader.translation = shaderTranslator().translateToMSL(shader.stage, shader.glslSource,
                                                           shader.options);
    shader.translated = true;
    return shader.translation.succeeded;
}
#endif

bool populateDescriptorShaders(const StateSnapshot& snapshot, PipelineDescriptor& descriptor) {
#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
    auto& shaders = shaderMapStorage();

    if (snapshot.shaders.vertexShader != 0) {
        auto vertexIt = shaders.find(snapshot.shaders.vertexShader);
        if (vertexIt == shaders.end() || vertexIt->second.stage != ShaderStage::Vertex ||
            !translateShader(vertexIt->second)) {
            return false;
        }
        descriptor.vertexMSL = vertexIt->second.translation.output.msl;
        descriptor.vertexFunction =
            inferStageFunctionName(descriptor.vertexMSL, ShaderStage::Vertex,
                                   vertexIt->second.translation.output.entryPoint);
    }

    if (snapshot.shaders.fragmentShader != 0) {
        auto fragmentIt = shaders.find(snapshot.shaders.fragmentShader);
        if (fragmentIt == shaders.end() || fragmentIt->second.stage != ShaderStage::Fragment ||
            !translateShader(fragmentIt->second)) {
            return false;
        }
        descriptor.fragmentMSL = fragmentIt->second.translation.output.msl;
        descriptor.fragmentFunction =
            inferStageFunctionName(descriptor.fragmentMSL, ShaderStage::Fragment,
                                   fragmentIt->second.translation.output.entryPoint);
    }
#endif

    return true;
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

jboolean MetalCraftJNI_RegisterShaderSource(jlong shaderId, jint stage, const char* glslSource,
                                            jboolean flipVertexY) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    if (shaderId == 0 || glslSource == nullptr || glslSource[0] == '\0') {
        return JNI_FALSE;
    }

    metalcraft::detail::RegisteredShader shader{};
    shader.glslSource = glslSource;
#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
    shader.stage = metalcraft::detail::toShaderStage(stage);
    shader.options.flipVertexY = flipVertexY != 0;
    shader.options.fixupClipSpace = true;
    shader.options.targetIOS = true;
#else
    shader.stage = stage;
    shader.flipVertexY = flipVertexY != 0;
#endif

    metalcraft::detail::shaderMapStorage()[static_cast<std::uint64_t>(shaderId)] = std::move(shader);
    return JNI_TRUE;
}

jlong MetalCraftJNI_AcquireCurrentPipeline() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::IRenderDevice* device = metalcraft::detail::ensureDevice();
    if (device == nullptr) {
        return 0;
    }

    metalcraft::PipelineDescriptor descriptor{};
    descriptor.snapshot = metalcraft::GetSharedStateTracker().snapshot();
    descriptor.key = metalcraft::GetSharedStateTracker().currentPipelineKey();
    if (!metalcraft::detail::populateDescriptorShaders(descriptor.snapshot, descriptor)) {
        return 0;
    }

    return static_cast<jlong>(device->acquirePipeline(descriptor));
}

jlong MetalCraftJNI_CurrentPipelineHash() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    return static_cast<jlong>(metalcraft::GetSharedStateTracker().currentPipelineKey().value);
}

void MetalCraftJNI_BeginFrame() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::IRenderDevice* device = metalcraft::detail::ensureDevice();
    if (device != nullptr) {
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

jboolean MetalCraftJNI_DrawMulti(jint topology, jlong commandsPtr, jint drawCount, jint stride) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    if (commandsPtr == 0 || drawCount <= 0) {
        return JNI_FALSE;
    }

    auto& device = metalcraft::detail::deviceStorage();
    if (!device || !device->supportsIndirectDraw()) {
        return JNI_FALSE;
    }

    const std::size_t commandStride =
        stride <= 0 ? sizeof(metalcraft::IndirectDrawArraysCommand)
                    : static_cast<std::size_t>(stride);
    const auto* base = reinterpret_cast<const std::uint8_t*>(commandsPtr);
    std::vector<metalcraft::IndirectDrawArraysCommand> commands;
    commands.reserve(static_cast<std::size_t>(drawCount));

    for (jint index = 0; index < drawCount; ++index) {
        const auto* command = reinterpret_cast<const metalcraft::IndirectDrawArraysCommand*>(
            base + static_cast<std::size_t>(index) * commandStride);
        commands.push_back(*command);
    }

    metalcraft::IndirectDrawBatch batch{};
    batch.topology = metalcraft::detail::toTopology(topology);
    batch.commands = commands.data();
    batch.commandCount = commands.size();
    batch.stride = commandStride;
    device->drawIndirect(batch);
    return JNI_TRUE;
}

jlong MetalCraftJNI_GetDrawCallCount() {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    auto& device = metalcraft::detail::deviceStorage();
    if (!device) {
        return 0;
    }
    return static_cast<jlong>(device->drawCallCount());
}

jlong MetalCraftJNI_CreateTexture(jint width, jint height, jint format, jboolean mipmapped) {
    std::lock_guard<std::mutex> lock(metalcraft::detail::sharedMutex());
    metalcraft::IRenderDevice* device = metalcraft::detail::ensureDevice();
    if (device == nullptr) {
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

JNIEXPORT jboolean JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nRegisterShaderSource(
    JNIEnv* env, jclass, jlong shaderId, jint stage, jstring glslSource, jboolean flipVertexY) {
    if (glslSource == nullptr) {
        return JNI_FALSE;
    }

    const char* sourceChars = env->GetStringUTFChars(glslSource, nullptr);
    if (sourceChars == nullptr) {
        return JNI_FALSE;
    }

    const jboolean result =
        MetalCraftJNI_RegisterShaderSource(shaderId, stage, sourceChars, flipVertexY);
    env->ReleaseStringUTFChars(glslSource, sourceChars);
    return result;
}

JNIEXPORT jlong JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nAcquireCurrentPipeline(
    JNIEnv*, jclass) {
    return MetalCraftJNI_AcquireCurrentPipeline();
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

JNIEXPORT jboolean JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nDrawMulti(
    JNIEnv*, jclass, jint topology, jlong commandsPtr, jint drawCount, jint stride) {
    return MetalCraftJNI_DrawMulti(topology, commandsPtr, drawCount, stride);
}

JNIEXPORT jlong JNICALL Java_net_kdt_pojavlaunch_render_MetalCraftBridge_nGetDrawCallCount(
    JNIEnv*, jclass) {
    return MetalCraftJNI_GetDrawCallCount();
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
