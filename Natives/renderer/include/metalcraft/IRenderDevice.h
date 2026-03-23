#pragma once

#include "metalcraft/IBuffer.h"
#include "metalcraft/ITexture.h"
#include "metalcraft/RenderTypes.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace metalcraft {

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    virtual BufferPtr createBuffer(const BufferDescriptor& descriptor) = 0;
    virtual TexturePtr createTexture(const TextureDescriptor& descriptor) = 0;
    virtual std::uint64_t acquirePipeline(const PipelineDescriptor& descriptor) = 0;
    virtual std::uint64_t acquireDepthStencilState(const DepthState& state) = 0;
    virtual bool writeRing(std::size_t size, std::size_t alignment, RingAllocation& allocation) = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void draw(const DrawCallInfo& info) = 0;
    virtual void drawIndexed(const DrawCallInfo& info) = 0;
    virtual void drawIndirect(const IndirectDrawBatch& batch) = 0;
    virtual void submit() = 0;
    virtual bool supportsIndirectDraw() const noexcept = 0;
    virtual std::size_t ringBufferCapacity() const noexcept = 0;
    virtual std::size_t drawCallCount() const noexcept = 0;
};

std::unique_ptr<IRenderDevice> CreateRenderDevice(std::size_t ringBufferCapacity);

} // namespace metalcraft
