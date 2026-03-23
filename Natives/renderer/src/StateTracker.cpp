#include "metalcraft/StateTracker.h"

namespace metalcraft {
namespace {

constexpr std::uint64_t kHashSeed = 0xCBF29CE484222325ULL;

std::uint64_t mix(std::uint64_t value) noexcept {
    value ^= value >> 30U;
    value *= 0xBF58476D1CE4E5B9ULL;
    value ^= value >> 27U;
    value *= 0x94D049BB133111EBULL;
    value ^= value >> 31U;
    return value;
}

std::uint64_t combine(std::uint64_t seed, std::uint64_t value) noexcept {
    return seed ^ (mix(value) + 0x9E3779B97F4A7C15ULL + (seed << 6U) + (seed >> 2U));
}

} // namespace

std::uint64_t HashStateSnapshot(const StateSnapshot& snapshot) noexcept {
    std::uint64_t seed = kHashSeed;

    seed = combine(seed, snapshot.blend.enabled ? 1U : 0U);
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.blend.srcColor));
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.blend.dstColor));
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.blend.colorOp));
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.blend.srcAlpha));
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.blend.dstAlpha));
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.blend.alphaOp));
    seed = combine(seed, snapshot.blend.writeMask);

    seed = combine(seed, snapshot.depth.testEnabled ? 1U : 0U);
    seed = combine(seed, snapshot.depth.writeEnabled ? 1U : 0U);
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.depth.compareOp));

    seed = combine(seed, snapshot.shaders.vertexShader);
    seed = combine(seed, snapshot.shaders.fragmentShader);
    seed = combine(seed, snapshot.shaders.vertexLayout);

    seed = combine(seed, static_cast<std::uint64_t>(snapshot.topology));
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.formats.colorFormat));
    seed = combine(seed, static_cast<std::uint64_t>(snapshot.formats.depthFormat));
    seed = combine(seed, snapshot.formats.sampleCount);

    return seed == 0 ? 1 : seed;
}

StateTracker::StateTracker() {
    reset();
}

void StateTracker::reset() {
    snapshot_ = StateSnapshot{};
    markDirty();
}

void StateTracker::setBlendEnabled(bool enabled) {
    if (snapshot_.blend.enabled == enabled) {
        return;
    }
    snapshot_.blend.enabled = enabled;
    markDirty();
}

void StateTracker::setBlendFactors(BlendFactor srcColor, BlendFactor dstColor,
                                   BlendFactor srcAlpha, BlendFactor dstAlpha,
                                   BlendOp colorOp, BlendOp alphaOp) {
    if (snapshot_.blend.srcColor == srcColor && snapshot_.blend.dstColor == dstColor &&
        snapshot_.blend.srcAlpha == srcAlpha && snapshot_.blend.dstAlpha == dstAlpha &&
        snapshot_.blend.colorOp == colorOp && snapshot_.blend.alphaOp == alphaOp) {
        return;
    }
    snapshot_.blend.srcColor = srcColor;
    snapshot_.blend.dstColor = dstColor;
    snapshot_.blend.srcAlpha = srcAlpha;
    snapshot_.blend.dstAlpha = dstAlpha;
    snapshot_.blend.colorOp = colorOp;
    snapshot_.blend.alphaOp = alphaOp;
    markDirty();
}

void StateTracker::setBlendWriteMask(std::uint8_t writeMask) {
    if (snapshot_.blend.writeMask == writeMask) {
        return;
    }
    snapshot_.blend.writeMask = writeMask;
    markDirty();
}

void StateTracker::setDepthState(bool testEnabled, bool writeEnabled, CompareOp compareOp) {
    if (snapshot_.depth.testEnabled == testEnabled &&
        snapshot_.depth.writeEnabled == writeEnabled &&
        snapshot_.depth.compareOp == compareOp) {
        return;
    }
    snapshot_.depth.testEnabled = testEnabled;
    snapshot_.depth.writeEnabled = writeEnabled;
    snapshot_.depth.compareOp = compareOp;
    markDirty();
}

void StateTracker::bindShaders(std::uint64_t vertexShader, std::uint64_t fragmentShader,
                               std::uint64_t vertexLayout) {
    if (snapshot_.shaders.vertexShader == vertexShader &&
        snapshot_.shaders.fragmentShader == fragmentShader &&
        snapshot_.shaders.vertexLayout == vertexLayout) {
        return;
    }
    snapshot_.shaders.vertexShader = vertexShader;
    snapshot_.shaders.fragmentShader = fragmentShader;
    snapshot_.shaders.vertexLayout = vertexLayout;
    markDirty();
}

void StateTracker::setTopology(PrimitiveTopology topology) {
    if (snapshot_.topology == topology) {
        return;
    }
    snapshot_.topology = topology;
    markDirty();
}

void StateTracker::setRenderPassFormats(PixelFormat colorFormat, PixelFormat depthFormat,
                                        std::uint8_t sampleCount) {
    const std::uint8_t normalizedSampleCount = sampleCount == 0 ? 1 : sampleCount;
    if (snapshot_.formats.colorFormat == colorFormat &&
        snapshot_.formats.depthFormat == depthFormat &&
        snapshot_.formats.sampleCount == normalizedSampleCount) {
        return;
    }
    snapshot_.formats.colorFormat = colorFormat;
    snapshot_.formats.depthFormat = depthFormat;
    snapshot_.formats.sampleCount = normalizedSampleCount;
    markDirty();
}

const StateSnapshot& StateTracker::snapshot() const noexcept {
    return snapshot_;
}

PipelineKey StateTracker::currentPipelineKey() const noexcept {
    return pipelineKey_;
}

bool StateTracker::dirty() const noexcept {
    return dirty_;
}

void StateTracker::clearDirty() noexcept {
    dirty_ = false;
}

void StateTracker::markDirty() noexcept {
    pipelineKey_.value = HashStateSnapshot(snapshot_);
    dirty_ = true;
}

} // namespace metalcraft
