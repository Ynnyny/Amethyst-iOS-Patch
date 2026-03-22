#pragma once

namespace metalcraft {

/// Forward-declared opaque handle for Metal command encoder.
/// Only compiled on Apple; on Linux this header is unused.
#ifdef __APPLE__

void* MetalCommandEncoderCreate(const void* commandQueueHandle);
void MetalCommandEncoderBeginFrame(void* encoderHandle);
void MetalCommandEncoderEndFrame(void* encoderHandle);
void MetalCommandEncoderDestroy(void* encoderHandle);

#endif

} // namespace metalcraft
