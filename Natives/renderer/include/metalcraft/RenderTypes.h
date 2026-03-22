#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace metalcraft {

enum class BufferUsage : std::uint8_t {
    Vertex = 0,
    Index,
    Uniform,
    Upload,
    Indirect
};

enum class CompareOp : std::uint8_t {
    Never = 0,
    Less,
    LessEqual,
    Equal,
    Greater,
    GreaterEqual,
    Always
};

enum class BlendFactor : std::uint8_t {
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstColor,
    OneMinusDstColor,
    DstAlpha,
    OneMinusDstAlpha
};

enum class BlendOp : std::uint8_t {
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

enum class PrimitiveTopology : std::uint8_t {
    Triangles = 0,
    TriangleStrip,
    Lines,
    LineStrip,
    Points
};

enum class PixelFormat : std::uint16_t {
    Invalid = 0,
    BGRA8Unorm = 80,
    RGBA16Float = 112,
    Depth32Float = 252,
    Depth24Stencil8 = 255
};

struct BufferDescriptor {
    std::size_t size = 0;
    BufferUsage usage = BufferUsage::Vertex;
    bool cpuVisible = false;
};

struct BlendState {
    bool enabled = false;
    BlendFactor srcColor = BlendFactor::One;
    BlendFactor dstColor = BlendFactor::Zero;
    BlendOp colorOp = BlendOp::Add;
    BlendFactor srcAlpha = BlendFactor::One;
    BlendFactor dstAlpha = BlendFactor::Zero;
    BlendOp alphaOp = BlendOp::Add;
    std::uint8_t writeMask = 0x0F;

    bool operator==(const BlendState& other) const noexcept {
        return enabled == other.enabled &&
               srcColor == other.srcColor &&
               dstColor == other.dstColor &&
               colorOp == other.colorOp &&
               srcAlpha == other.srcAlpha &&
               dstAlpha == other.dstAlpha &&
               alphaOp == other.alphaOp &&
               writeMask == other.writeMask;
    }
};

struct DepthState {
    bool testEnabled = false;
    bool writeEnabled = true;
    CompareOp compareOp = CompareOp::LessEqual;

    bool operator==(const DepthState& other) const noexcept {
        return testEnabled == other.testEnabled &&
               writeEnabled == other.writeEnabled &&
               compareOp == other.compareOp;
    }
};

struct ShaderBinding {
    std::uint64_t vertexShader = 0;
    std::uint64_t fragmentShader = 0;
    std::uint64_t vertexLayout = 0;

    bool operator==(const ShaderBinding& other) const noexcept {
        return vertexShader == other.vertexShader &&
               fragmentShader == other.fragmentShader &&
               vertexLayout == other.vertexLayout;
    }
};

struct RenderPassFormats {
    PixelFormat colorFormat = PixelFormat::BGRA8Unorm;
    PixelFormat depthFormat = PixelFormat::Depth32Float;
    std::uint8_t sampleCount = 1;

    bool operator==(const RenderPassFormats& other) const noexcept {
        return colorFormat == other.colorFormat &&
               depthFormat == other.depthFormat &&
               sampleCount == other.sampleCount;
    }
};

struct StateSnapshot {
    BlendState blend{};
    DepthState depth{};
    ShaderBinding shaders{};
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    RenderPassFormats formats{};

    bool operator==(const StateSnapshot& other) const noexcept {
        return blend == other.blend &&
               depth == other.depth &&
               shaders == other.shaders &&
               topology == other.topology &&
               formats == other.formats;
    }
};

struct PipelineKey {
    std::uint64_t value = 0;

    bool operator==(const PipelineKey& other) const noexcept {
        return value == other.value;
    }
};

struct PipelineDescriptor {
    StateSnapshot snapshot{};
    PipelineKey key{};
    std::string vertexFunction = "main0";
    std::string fragmentFunction = "main0";
    std::string vertexMSL{};
    std::string fragmentMSL{};
};

struct RingAllocation {
    std::size_t offset = 0;
    std::size_t size = 0;
    std::uint8_t* cpuAddress = nullptr;
};

struct TextureDescriptor {
    std::uint32_t width = 1;
    std::uint32_t height = 1;
    PixelFormat format = PixelFormat::BGRA8Unorm;
    bool mipmapped = false;
};

struct DrawCallInfo {
    PrimitiveTopology topology = PrimitiveTopology::Triangles;
    std::uint32_t vertexStart = 0;
    std::uint32_t vertexCount = 0;
    std::uint32_t indexCount = 0;
    std::uint32_t instanceCount = 1;
    std::size_t indexBufferOffset = 0;
};

} // namespace metalcraft
