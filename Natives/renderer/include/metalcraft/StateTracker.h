#pragma once

#include "metalcraft/RenderTypes.h"

#include <cstdint>

namespace metalcraft {

std::uint64_t HashStateSnapshot(const StateSnapshot& snapshot) noexcept;

class StateTracker {
public:
    StateTracker();

    void reset();
    void setBlendEnabled(bool enabled);
    void setBlendFactors(BlendFactor srcColor, BlendFactor dstColor, BlendFactor srcAlpha,
                         BlendFactor dstAlpha, BlendOp colorOp, BlendOp alphaOp);
    void setBlendWriteMask(std::uint8_t writeMask);
    void setDepthState(bool testEnabled, bool writeEnabled, CompareOp compareOp);
    void bindShaders(std::uint64_t vertexShader, std::uint64_t fragmentShader,
                     std::uint64_t vertexLayout);
    void setTopology(PrimitiveTopology topology);
    void setRenderPassFormats(PixelFormat colorFormat, PixelFormat depthFormat,
                              std::uint8_t sampleCount);

    const StateSnapshot& snapshot() const noexcept;
    PipelineKey currentPipelineKey() const noexcept;
    bool dirty() const noexcept;
    void clearDirty() noexcept;

private:
    void markDirty() noexcept;

    StateSnapshot snapshot_{};
    PipelineKey pipelineKey_{};
    bool dirty_ = true;
};

} // namespace metalcraft
