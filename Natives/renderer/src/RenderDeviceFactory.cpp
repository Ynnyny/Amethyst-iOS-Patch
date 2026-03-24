#include "metalcraft/IRenderDevice.h"

namespace metalcraft {

#ifdef __APPLE__
std::unique_ptr<IRenderDevice> CreateMetalRenderDevice(std::size_t ringBufferCapacity);
#endif

std::unique_ptr<IRenderDevice> CreateRenderDevice(std::size_t ringBufferCapacity) {
#ifdef __APPLE__
    return CreateMetalRenderDevice(ringBufferCapacity);
#else
    (void)ringBufferCapacity;
    return nullptr;
#endif
}

} // namespace metalcraft
