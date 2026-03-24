#include "metalcraft/IRenderDevice.h"
#include "metalcraft/ITexture.h"
#include "metalcraft/JNIBridge.h"
#include "metalcraft/RingBuffer.h"
#include "metalcraft/ShaderTranslator.h"
#include "metalcraft/StateTracker.h"

#include <array>
#include <cstdint>
#include <cstring>

int main() {
    using namespace metalcraft;

    const char* vertexSource = R"(#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 0) out vec2 vUv;
void main() {
    vUv = aUv;
	gl_Position = vec4(aPos, 1.0);
})";

    const char* fragmentSource = R"(#version 450 core
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 vUv;
void main() {
    fragColor = vec4(vUv, 0.5, 1.0);
})";

    // ====== Test 1-2: State Tracker Hash ======
    StateTracker tracker;
    const std::uint64_t baseHash = tracker.currentPipelineKey().value;
    tracker.setBlendEnabled(true);
    const std::uint64_t blendHash = tracker.currentPipelineKey().value;
    if (baseHash == blendHash) {
        return 1;
    }

    tracker.setBlendEnabled(false);
    if (tracker.currentPipelineKey().value != baseHash) {
        return 2;
    }
    tracker.clearDirty();
    tracker.setBlendEnabled(false);
    if (tracker.dirty()) {
        return 3;
    }
    tracker.setBlendEnabled(true);
    if (!tracker.dirty()) {
        return 4;
    }
    tracker.clearDirty();
    tracker.setRenderPassFormats(PixelFormat::BGRA8Unorm, PixelFormat::Depth32Float, 1);
    if (tracker.dirty()) {
        return 5;
    }

    // ====== Test 6-8: Ring Buffer ======
    RingBuffer ringBuffer(256);
    RingAllocation ringA{};
    RingAllocation ringB{};
    if (!ringBuffer.allocate(48, 16, ringA) || ringA.offset != 0) {
        return 6;
    }
    if (!ringBuffer.allocate(32, 64, ringB) || ringB.offset != 64) {
        return 7;
    }
    ringBuffer.beginFrame();
    RingAllocation ringC{};
    if (!ringBuffer.allocate(128, 32, ringC) || ringC.offset != 0) {
        return 8;
    }

    // ====== Test 9: Device Creation ======
    auto device = CreateRenderDevice(256);
    if (!device) {
        return 9;
    }
    if (!device->isReady()) {
        return 10;
    }

    // ====== Test 11-12: Pipeline Cache ======
    PipelineDescriptor descriptor{};
    descriptor.snapshot = tracker.snapshot();
    descriptor.key = tracker.currentPipelineKey();

    const std::uint64_t pipelineA = device->acquirePipeline(descriptor);
    const std::uint64_t pipelineB = device->acquirePipeline(descriptor);
    if (pipelineA == 0 || pipelineA != pipelineB) {
        return 11;
    }

    tracker.bindShaders(11, 22, 33);
    descriptor.snapshot = tracker.snapshot();
    descriptor.key = tracker.currentPipelineKey();

    const std::uint64_t pipelineC = device->acquirePipeline(descriptor);
    if (pipelineC == 0 || pipelineC == pipelineA) {
        return 12;
    }

    // ====== Test 13: JNI Device Support ======
    if (MetalCraftJNI_IsSupported() == JNI_FALSE) {
        return 13;
    }

    // ====== Test 14-16: Ring Buffer on Device ======
    RingAllocation uploadA{};
    RingAllocation uploadB{};
    if (!device->writeRing(80, 32, uploadA) || uploadA.offset != 0) {
        return 14;
    }
    if (!device->writeRing(64, 64, uploadB) || uploadB.offset != 128) {
        return 15;
    }
    device->beginFrame();
    RingAllocation uploadC{};
    if (!device->writeRing(64, 16, uploadC) || uploadC.offset != 0) {
        return 16;
    }

    // ====== Test 17: JNI State Tracker ======
    MetalCraftJNI_ResetStateTracker();
    const std::uint64_t jniBaseHash = static_cast<std::uint64_t>(MetalCraftJNI_CurrentPipelineHash());
    MetalCraftJNI_SetBlendState(
        1,
        static_cast<jint>(BlendFactor::SrcAlpha),
        static_cast<jint>(BlendFactor::OneMinusSrcAlpha),
        static_cast<jint>(BlendFactor::One),
        static_cast<jint>(BlendFactor::Zero),
        static_cast<jint>(BlendOp::Add),
        static_cast<jint>(BlendOp::Add),
        0x0F);
    MetalCraftJNI_SetDepthState(1, 1, static_cast<jint>(CompareOp::LessEqual));
    MetalCraftJNI_BindShaders(101, 202, 303);
    MetalCraftJNI_SetRenderPassState(
        static_cast<jint>(PixelFormat::BGRA8Unorm),
        static_cast<jint>(PixelFormat::Depth32Float),
        1,
        static_cast<jint>(PrimitiveTopology::Triangles));

    const std::uint64_t jniMutatedHash =
        static_cast<std::uint64_t>(MetalCraftJNI_CurrentPipelineHash());
    if (jniBaseHash == jniMutatedHash) {
        return 17;
    }
    if (MetalCraftJNI_RegisterShaderSource(
            101, static_cast<jint>(ShaderStage::Vertex), vertexSource,
            JNI_FALSE) == JNI_FALSE) {
        return 61;
    }
    if (MetalCraftJNI_RegisterShaderSource(
            202, static_cast<jint>(ShaderStage::Fragment), fragmentSource,
            JNI_FALSE) == JNI_FALSE) {
        return 62;
    }

    // ====== Test 18-20: Texture Creation ======
    TextureDescriptor texDesc{};
    texDesc.width = 64;
    texDesc.height = 64;
    texDesc.format = PixelFormat::BGRA8Unorm;
    auto texture = device->createTexture(texDesc);
    if (!texture) {
        return 18;
    }
    if (texture->width() != 64 || texture->height() != 64) {
        return 19;
    }

    TextureDescriptor invalidTexDesc{};
    invalidTexDesc.width = 16;
    invalidTexDesc.height = 16;
    invalidTexDesc.format = PixelFormat::Invalid;
    if (device->createTexture(invalidTexDesc)) {
        return 20;
    }

    // ====== Test 21-22: Texture Upload ======
    const std::size_t pixelDataSize = 64 * 64 * 4;
    std::vector<std::uint8_t> pixels(pixelDataSize, 0xFF);
    if (!texture->upload(0, 0, 64, 64, pixels.data(), 64 * 4)) {
        return 21;
    }
    if (texture->upload(0, 0, 64, 64, pixels.data(), 63 * 4)) {
        return 22;
    }

    // ====== Test 23-24: DepthStencil State ======
    DepthState depthState{};
    depthState.testEnabled = true;
    depthState.writeEnabled = true;
    depthState.compareOp = CompareOp::LessEqual;
    const std::uint64_t dsA = device->acquireDepthStencilState(depthState);
    const std::uint64_t dsB = device->acquireDepthStencilState(depthState);
    if (dsA == 0 || dsA != dsB) {
        return 23;
    }

    depthState.compareOp = CompareOp::Always;
    const std::uint64_t dsC = device->acquireDepthStencilState(depthState);
    if (dsC == 0 || dsC == dsA) {
        return 24;
    }
    depthState.compareOp = CompareOp::NotEqual;
    const std::uint64_t dsD = device->acquireDepthStencilState(depthState);
    if (dsD == 0 || dsD == dsA || dsD == dsC) {
        return 59;
    }

    BoundRenderState boundState{};
    boundState.pipelineHandle = pipelineC;
    boundState.depthStencilHandle = dsA;
    boundState.snapshot = descriptor.snapshot;
    device->bindRenderState(boundState);
    const BoundRenderState currentState = device->currentRenderState();
    if (!currentState.valid() || currentState.pipelineHandle != pipelineC ||
        currentState.depthStencilHandle != dsA) {
        return 60;
    }

    // ====== Test 25-27: Draw Call Tracking ======
    device->beginFrame();
    if (device->drawCallCount() != 0) {
        return 25;
    }

    DrawCallInfo drawInfo{};
    drawInfo.topology = PrimitiveTopology::Triangles;
    drawInfo.vertexStart = 0;
    drawInfo.vertexCount = 36;
    drawInfo.instanceCount = 1;
    device->draw(drawInfo);
    device->draw(drawInfo);

    DrawCallInfo indexedInfo{};
    indexedInfo.topology = PrimitiveTopology::Triangles;
    indexedInfo.indexCount = 36;
    indexedInfo.indexBufferOffset = 0;
    indexedInfo.instanceCount = 1;
    device->drawIndexed(indexedInfo);

    if (device->drawCallCount() != 3) {
        return 26;
    }

    IndirectDrawArraysCommand indirectCommands[] = {
        {24, 1, 0, 0},
        {12, 2, 24, 0},
    };
    IndirectDrawBatch indirectBatch{};
    indirectBatch.topology = PrimitiveTopology::Triangles;
    indirectBatch.commands = indirectCommands;
    indirectBatch.commandCount = 2;
    indirectBatch.stride = sizeof(IndirectDrawArraysCommand);
    device->drawIndirect(indirectBatch);

    if (!device->supportsIndirectDraw() || device->drawCallCount() != 5) {
        return 27;
    }

    device->endFrame();

    // ====== Test 28-31: JNI Frame & Draw ======
    MetalCraftJNI_BeginFrame();
    if (MetalCraftJNI_AcquireCurrentPipeline() == 0) {
        return 63;
    }
    MetalCraftJNI_Draw(static_cast<jint>(PrimitiveTopology::Triangles), 0, 36, 1);
    MetalCraftJNI_DrawIndexed(static_cast<jint>(PrimitiveTopology::Triangles), 36, 0, 1);
    if (MetalCraftJNI_GetDrawCallCount() != 2) {
        return 28;
    }
    if (MetalCraftJNI_DrawMulti(static_cast<jint>(PrimitiveTopology::Triangles),
                                reinterpret_cast<jlong>(indirectCommands), 2,
                                sizeof(IndirectDrawArraysCommand)) == JNI_FALSE) {
        return 29;
    }
    if (MetalCraftJNI_GetDrawCallCount() != 4) {
        return 30;
    }
    MetalCraftJNI_EndFrame();

    MetalCraftJNI_BeginFrame();
    MetalCraftJNI_Draw(static_cast<jint>(PrimitiveTopology::Triangles), -1, 36, 1);
    MetalCraftJNI_DrawIndexed(static_cast<jint>(PrimitiveTopology::Triangles), 36, -1, 1);
    if (MetalCraftJNI_GetDrawCallCount() != 0) {
        return 31;
    }
    MetalCraftJNI_EndFrame();

    // ====== Additional Buffer Coverage ======
    BufferDescriptor bufferDesc{};
    bufferDesc.size = 64;
    bufferDesc.usage = BufferUsage::Vertex;
    bufferDesc.cpuVisible = true;
    auto buffer = device->createBuffer(bufferDesc);
    if (!buffer || buffer->size() != 64) {
        return 46;
    }
    std::array<std::uint8_t, 16> bufferBytes{};
    bufferBytes.fill(0x3C);
    if (!buffer->update(0, bufferBytes.data(), bufferBytes.size())) {
        return 47;
    }
    if (buffer->update(60, bufferBytes.data(), bufferBytes.size())) {
        return 48;
    }
    if (MetalCraftJNI_CreateBuffer(0, static_cast<jint>(BufferUsage::Vertex), JNI_TRUE) != 0) {
        return 49;
    }
    if (MetalCraftJNI_CreateBuffer(32, 999, JNI_TRUE) != 0) {
        return 50;
    }
    const jlong bufferId = MetalCraftJNI_CreateBuffer(
        32, static_cast<jint>(BufferUsage::Vertex), JNI_TRUE);
    if (bufferId == 0) {
        return 51;
    }
    if (MetalCraftJNI_UpdateBuffer(bufferId, 0,
                                   reinterpret_cast<jlong>(bufferBytes.data()),
                                   bufferBytes.size()) == JNI_FALSE) {
        return 52;
    }
    if (MetalCraftJNI_UpdateBuffer(bufferId, 24,
                                   reinterpret_cast<jlong>(bufferBytes.data()),
                                   bufferBytes.size()) != JNI_FALSE) {
        return 53;
    }
    if (MetalCraftJNI_DestroyBuffer(bufferId) == JNI_FALSE) {
        return 54;
    }
    if (MetalCraftJNI_DestroyBuffer(bufferId) != JNI_FALSE) {
        return 55;
    }

    // ====== Test 29-33: JNI Texture ======
    const jlong texId = MetalCraftJNI_CreateTexture(
        32, 32, static_cast<jint>(PixelFormat::BGRA8Unorm), JNI_FALSE);
    if (texId == 0) {
        return 29;
    }

    std::vector<std::uint8_t> texPixels(32 * 32 * 4, 0xAA);
    const jboolean uploadResult = MetalCraftJNI_UploadTexture(
        texId, 0, 0, 32, 32,
        reinterpret_cast<jlong>(texPixels.data()), 32 * 4);
    if (uploadResult == JNI_FALSE) {
        return 30;
    }
    if (MetalCraftJNI_DestroyTexture(texId) == JNI_FALSE) {
        return 31;
    }
    if (MetalCraftJNI_DestroyTexture(texId) != JNI_FALSE) {
        return 32;
    }
    const jlong recreatedTexId = MetalCraftJNI_CreateTexture(
        32, 32, static_cast<jint>(PixelFormat::BGRA8Unorm), JNI_FALSE);
    if (recreatedTexId == 0) {
        return 33;
    }
    if (MetalCraftJNI_CreateTexture(4, 4, static_cast<jint>(PixelFormat::Invalid), JNI_FALSE) != 0) {
        return 34;
    }
    if (MetalCraftJNI_CreateTexture(-1, 4, static_cast<jint>(PixelFormat::BGRA8Unorm), JNI_FALSE) != 0) {
        return 35;
    }
    if (MetalCraftJNI_UploadTexture(recreatedTexId, -1, 0, 32, 32,
                                    reinterpret_cast<jlong>(texPixels.data()),
                                    32 * 4) != JNI_FALSE) {
        return 36;
    }
    const jlong shutdownBufferId = MetalCraftJNI_CreateBuffer(
        32, static_cast<jint>(BufferUsage::Index), JNI_TRUE);
    if (shutdownBufferId == 0) {
        return 56;
    }
    MetalCraftJNI_Shutdown();
    if (MetalCraftJNI_GetDrawCallCount() != 0) {
        return 37;
    }
    if (MetalCraftJNI_DestroyBuffer(shutdownBufferId) != JNI_FALSE) {
        return 57;
    }
    if (MetalCraftJNI_IsSupported() == JNI_FALSE) {
        return 38;
    }
#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
    MetalCraftJNI_BindShaders(501, 502, 901);
    MetalCraftJNI_SetRenderPassState(static_cast<jint>(PixelFormat::BGRA8Unorm),
                                     static_cast<jint>(PixelFormat::Depth32Float), 1,
                                     static_cast<jint>(PrimitiveTopology::Triangles));
    if (MetalCraftJNI_AcquireCurrentPipeline() != 0) {
        return 39;
    }
#endif

    // ====== Test 40-45: Shader Translation & Pipeline Assembly ======
#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
    ShaderTranslator translator;
    const ShaderTranslationResult vertexTranslation = translator.translateToMSL(
        ShaderStage::Vertex, vertexSource);
    if (!vertexTranslation.succeeded || vertexTranslation.output.spirv.empty()) {
        return 40;
    }
    if (vertexTranslation.output.msl.find("vertex") == std::string::npos ||
        vertexTranslation.output.msl.find("[[position]]") == std::string::npos) {
        return 41;
    }
    if (vertexTranslation.output.msl.find("0.5") == std::string::npos &&
        vertexTranslation.output.msl.find("gl_Position.z") == std::string::npos &&
        vertexTranslation.output.msl.find("position.z") == std::string::npos) {
        return 42;
    }

    if (MetalCraftJNI_RegisterShaderSource(
            501, static_cast<jint>(ShaderStage::Vertex), vertexSource,
            JNI_FALSE) == JNI_FALSE) {
        return 43;
    }
    if (MetalCraftJNI_RegisterShaderSource(
            502, static_cast<jint>(ShaderStage::Fragment), fragmentSource,
            JNI_FALSE) == JNI_FALSE) {
        return 44;
    }
    if (MetalCraftJNI_RegisterShaderSource(
            503, 999, fragmentSource, JNI_FALSE) != JNI_FALSE) {
        return 58;
    }

    MetalCraftJNI_ResetStateTracker();
    MetalCraftJNI_BindShaders(501, 502, 901);
    MetalCraftJNI_SetRenderPassState(static_cast<jint>(PixelFormat::BGRA8Unorm),
                                     static_cast<jint>(PixelFormat::Depth32Float), 1,
                                     static_cast<jint>(PrimitiveTopology::Triangles));
    if (MetalCraftJNI_AcquireCurrentPipeline() == 0) {
        return 45;
    }
#endif

    return 0;
}
