#pragma once

#include "metalcraft/IBuffer.h"
#include "metalcraft/ITexture.h"
#include "metalcraft/IRenderDevice.h"

#include <cstddef>
#include <memory>

namespace metalcraft {

BufferPtr CreateDummyBuffer(const BufferDescriptor& descriptor);
TexturePtr CreateDummyTexture(const TextureDescriptor& descriptor);
std::unique_ptr<IRenderDevice> CreateDummyRenderDevice(std::size_t ringBufferCapacity);

} // namespace metalcraft
