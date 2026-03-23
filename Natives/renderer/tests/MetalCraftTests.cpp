#include "metalcraft/IRenderDevice.h"
#include "metalcraft/ITexture.h"
#include "metalcraft/JNIBridge.h"
#include "metalcraft/RingBuffer.h"
#include "metalcraft/ShaderTranslator.h"
#include "metalcraft/StateTracker.h"

#include <cstdint>
#include <cstring>

int main() {
    using namespace metalcraft;

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

    // ====== Test 3-5: Ring Buffer ======
    RingBuffer ringBuffer(256);
    RingAllocation ringA{};
    RingAllocation ringB{};
    if (!ringBuffer.allocate(48, 16, ringA) || ringA.offset != 0) {
        return 3;
    }
    if (!ringBuffer.allocate(32, 64, ringB) || ringB.offset != 64) {
        return 4;
    }
    ringBuffer.beginFrame();
    RingAllocation ringC{};
    if (!ringBuffer.allocate(128, 32, ringC) || ringC.offset != 0) {
        return 5;
    }

    // ====== Test 6: Device Creation ======
    auto device = CreateRenderDevice(256);
    if (!device) {
        return 6;
    }

    // ====== Test 7-8: Pipeline Cache ======
    PipelineDescriptor descriptor{};
    descriptor.snapshot = tracker.snapshot();
    descriptor.key = tracker.currentPipelineKey();

    const std::uint64_t pipelineA = device->acquirePipeline(descriptor);
    const std::uint64_t pipelineB = device->acquirePipeline(descriptor);
    if (pipelineA == 0 || pipelineA != pipelineB) {
        return 7;
    }

    tracker.bindShaders(11, 22, 33);
    descriptor.snapshot = tracker.snapshot();
    descriptor.key = tracker.currentPipelineKey();

    const std::uint64_t pipelineC = device->acquirePipeline(descriptor);
    if (pipelineC == 0 || pipelineC == pipelineA) {
        return 8;
    }

    // ====== Test 9-11: Ring Buffer on Device ======
    RingAllocation uploadA{};
    RingAllocation uploadB{};
    if (!device->writeRing(80, 32, uploadA) || uploadA.offset != 0) {
        return 9;
    }
    if (!device->writeRing(64, 64, uploadB) || uploadB.offset != 128) {
        return 10;
    }
    device->beginFrame();
    RingAllocation uploadC{};
    if (!device->writeRing(64, 16, uploadC) || uploadC.offset != 0) {
        return 11;
    }

    // ====== Test 12: JNI State Tracker ======
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
        return 12;
    }

    // ====== Test 13: Texture Creation ======
    TextureDescriptor texDesc{};
    texDesc.width = 64;
    texDesc.height = 64;
    texDesc.format = PixelFormat::BGRA8Unorm;
    auto texture = device->createTexture(texDesc);
    if (!texture) {
        return 13;
    }
    if (texture->width() != 64 || texture->height() != 64) {
        return 14;
    }

    // ====== Test 15: Texture Upload ======
    const std::size_t pixelDataSize = 64 * 64 * 4;
    std::vector<std::uint8_t> pixels(pixelDataSize, 0xFF);
    if (!texture->upload(0, 0, 64, 64, pixels.data(), 64 * 4)) {
        return 15;
    }

    // ====== Test 16: DepthStencil State ======
    DepthState depthState{};
    depthState.testEnabled = true;
    depthState.writeEnabled = true;
    depthState.compareOp = CompareOp::LessEqual;
    const std::uint64_t dsA = device->acquireDepthStencilState(depthState);
    const std::uint64_t dsB = device->acquireDepthStencilState(depthState);
    if (dsA == 0 || dsA != dsB) {
        return 16;
    }

    depthState.compareOp = CompareOp::Always;
    const std::uint64_t dsC = device->acquireDepthStencilState(depthState);
    if (dsC == 0 || dsC == dsA) {
        return 17;
    }

    // ====== Test 18-19: Draw Call Tracking ======
    device->beginFrame();
    if (device->drawCallCount() != 0) {
        return 18;
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
        return 19;
    }

    device->endFrame();

    // ====== Test 20: JNI Frame & Draw ======
    MetalCraftJNI_BeginFrame();
    MetalCraftJNI_Draw(static_cast<jint>(PrimitiveTopology::Triangles), 0, 36, 1);
    MetalCraftJNI_DrawIndexed(static_cast<jint>(PrimitiveTopology::Triangles), 36, 0, 1);
    MetalCraftJNI_EndFrame();

    // ====== Test 21: JNI Texture ======
    const jlong texId = MetalCraftJNI_CreateTexture(
        32, 32, static_cast<jint>(PixelFormat::BGRA8Unorm), JNI_FALSE);
    if (texId == 0) {
        return 21;
    }

    std::vector<std::uint8_t> texPixels(32 * 32 * 4, 0xAA);
    const jboolean uploadResult = MetalCraftJNI_UploadTexture(
        texId, 0, 0, 32, 32,
        reinterpret_cast<jlong>(texPixels.data()), 32 * 4);
    if (uploadResult == JNI_FALSE) {
        return 22;
    }

    // ====== Test 23-25: Shader Translation ======
#ifdef METALCRAFT_HAS_SHADER_TRANSLATOR
    ShaderTranslator translator;
    const ShaderTranslationResult vertexTranslation = translator.translateToMSL(
        ShaderStage::Vertex,
        R"(#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUv;
layout(location = 0) out vec2 vUv;
void main() {
    vUv = aUv;
    gl_Position = vec4(aPos, 1.0);
})");
    if (!vertexTranslation.succeeded || vertexTranslation.output.spirv.empty()) {
        return 23;
    }
    if (vertexTranslation.output.msl.find("vertex") == std::string::npos ||
        vertexTranslation.output.msl.find("[[position]]") == std::string::npos) {
        return 24;
    }
    if (vertexTranslation.output.msl.find("0.5") == std::string::npos &&
        vertexTranslation.output.msl.find("gl_Position.z") == std::string::npos &&
        vertexTranslation.output.msl.find("position.z") == std::string::npos) {
        return 25;
    }
#endif

    return 0;
}
