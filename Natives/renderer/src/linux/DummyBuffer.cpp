#include "DummyBackend.h"

#ifdef __linux__

#include <cstring>
#include <vector>

namespace metalcraft {
namespace {

class DummyBuffer final : public IBuffer {
public:
    explicit DummyBuffer(const BufferDescriptor& descriptor)
        : descriptor_(descriptor),
          storage_(descriptor.size, 0) {}

    std::size_t size() const noexcept override {
        return descriptor_.size;
    }

    BufferUsage usage() const noexcept override {
        return descriptor_.usage;
    }

    bool update(std::size_t offset, const void* data, std::size_t length) override {
        if (data == nullptr || offset > storage_.size() || length > storage_.size() - offset) {
            return false;
        }
        std::memcpy(storage_.data() + offset, data, length);
        return true;
    }

    void* mappedData() noexcept override {
        return storage_.data();
    }

private:
    BufferDescriptor descriptor_{};
    std::vector<std::uint8_t> storage_;
};

} // namespace

BufferPtr CreateDummyBuffer(const BufferDescriptor& descriptor) {
    return std::make_shared<DummyBuffer>(descriptor);
}

} // namespace metalcraft

#endif
