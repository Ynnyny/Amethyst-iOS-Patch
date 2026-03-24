#include "DummyBackend.h"

#ifdef __linux__

#include "metalcraft/RingBuffer.h"

#include <unordered_map>

namespace metalcraft {
namespace {

class DummyRenderDevice final : public IRenderDevice {
public:
    explicit DummyRenderDevice(std::size_t ringBufferCapacity)
        : ringBuffer_(ringBufferCapacity) {}

    BufferPtr createBuffer(const BufferDescriptor& descriptor) override {
        return CreateDummyBuffer(descriptor);
    }

    TexturePtr createTexture(const TextureDescriptor& descriptor) override {
        return CreateDummyTexture(descriptor);
    }

    std::uint64_t acquirePipeline(const PipelineDescriptor& descriptor) override {
        const auto existing = pipelineCache_.find(descriptor.key.value);
        if (existing != pipelineCache_.end()) {
            return existing->second;
        }

        const std::uint64_t pipelineId = nextPipelineId_++;
        pipelineCache_.emplace(descriptor.key.value, pipelineId);
        return pipelineId;
    }

    std::uint64_t acquireDepthStencilState(const DepthState& state) override {
        std::uint64_t h = 0;
        h |= (state.testEnabled ? 1ULL : 0ULL);
        h |= (state.writeEnabled ? 2ULL : 0ULL);
        h |= (static_cast<std::uint64_t>(state.compareOp) << 2U);
        if (h == 0) h = 1;

        const auto existing = depthStencilCache_.find(h);
        if (existing != depthStencilCache_.end()) {
            return existing->second;
        }

        depthStencilCache_.emplace(h, h);
        return h;
    }

    void bindRenderState(const BoundRenderState& state) override {
        currentRenderState_ = state;
    }

    BoundRenderState currentRenderState() const noexcept override {
        return currentRenderState_;
    }

    bool writeRing(std::size_t size, std::size_t alignment, RingAllocation& allocation) override {
        return ringBuffer_.allocate(size, alignment, allocation);
    }

    void beginFrame() override {
        ringBuffer_.beginFrame();
        drawCallCount_ = 0;
    }

    void endFrame() override {}

    void draw(const DrawCallInfo& info) override {
        (void)info;
        if (!currentRenderState_.valid()) {
            return;
        }
        ++drawCallCount_;
    }

    void drawIndexed(const DrawCallInfo& info) override {
        (void)info;
        if (!currentRenderState_.valid()) {
            return;
        }
        ++drawCallCount_;
    }

    void drawIndirect(const IndirectDrawBatch& batch) override {
        if (!currentRenderState_.valid() || batch.commands == nullptr || batch.commandCount == 0) {
            return;
        }
        for (std::size_t index = 0; index < batch.commandCount; ++index) {
            const IndirectDrawArraysCommand& command = batch.commands[index];
            if (command.vertexCount == 0 || command.instanceCount == 0) {
                continue;
            }
            ++drawCallCount_;
        }
    }

    void submit() override {}

    bool isReady() const noexcept override {
        return true;
    }

    bool supportsIndirectDraw() const noexcept override {
        return true;
    }

    std::size_t ringBufferCapacity() const noexcept override {
        return ringBuffer_.capacity();
    }

    std::size_t drawCallCount() const noexcept override {
        return drawCallCount_;
    }

private:
    RingBuffer ringBuffer_;
    std::unordered_map<std::uint64_t, std::uint64_t> pipelineCache_;
    std::unordered_map<std::uint64_t, std::uint64_t> depthStencilCache_;
    BoundRenderState currentRenderState_{};
    std::uint64_t nextPipelineId_ = 1;
    std::size_t drawCallCount_ = 0;
};

} // namespace

std::unique_ptr<IRenderDevice> CreateDummyRenderDevice(std::size_t ringBufferCapacity) {
    return std::make_unique<DummyRenderDevice>(ringBufferCapacity);
}

} // namespace metalcraft

#endif
