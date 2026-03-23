#pragma once

#include "jni.h"
#include "metalcraft/StateTracker.h"

namespace metalcraft {

StateTracker& GetSharedStateTracker();

} // namespace metalcraft

extern "C" {

void MetalCraftJNI_ResetStateTracker();
void MetalCraftJNI_SetBlendState(jboolean enabled, jint srcColor, jint dstColor, jint srcAlpha,
                                 jint dstAlpha, jint colorOp, jint alphaOp, jint writeMask);
void MetalCraftJNI_SetDepthState(jboolean testEnabled, jboolean writeEnabled, jint compareOp);
void MetalCraftJNI_BindShaders(jlong vertexShaderId, jlong fragmentShaderId, jlong vertexLayoutId);
void MetalCraftJNI_SetRenderPassState(jint colorFormat, jint depthFormat, jint sampleCount,
                                      jint topology);
jboolean MetalCraftJNI_RegisterShaderSource(jlong shaderId, jint stage, const char* glslSource,
                                            jboolean flipVertexY);
jlong MetalCraftJNI_AcquireCurrentPipeline();
jlong MetalCraftJNI_CurrentPipelineHash();

void MetalCraftJNI_BeginFrame();
void MetalCraftJNI_EndFrame();
void MetalCraftJNI_Draw(jint topology, jint vertexStart, jint vertexCount, jint instanceCount);
void MetalCraftJNI_DrawIndexed(jint topology, jint indexCount, jlong indexBufferOffset,
                               jint instanceCount);
jboolean MetalCraftJNI_DrawMulti(jint topology, jlong commandsPtr, jint drawCount, jint stride);
jlong MetalCraftJNI_GetDrawCallCount();
jlong MetalCraftJNI_CreateTexture(jint width, jint height, jint format, jboolean mipmapped);
jboolean MetalCraftJNI_UploadTexture(jlong textureId, jint x, jint y, jint w, jint h,
                                     jlong dataPtr, jlong bytesPerRow);

} // extern "C"
