#include "metalcraft/IRenderDevice.h"

#ifdef __linux__
#include "linux/DummyBackend.h"
#endif

namespace metalcraft {

#ifdef __APPLE__
std::unique_ptr<IRenderDevice> CreateMetalRenderDevice(std::size_t ringBufferCapacity);
#endif

std::unique_ptr<IRenderDevice> CreateRenderDevice(std::size_t ringBufferCapacity) {
#ifdef __APPLE__
    return CreateMetalRenderDevice(ringBufferCapacity);
#elif defined(__linux__)
    return CreateDummyRenderDevice(ringBufferCapacity);
#else
    (void)ringBufferCapacity;
    return nullptr;
#endif
}

} // namespace metalcraft
