#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace metalcraft {

enum class ShaderStage : std::uint8_t {
    Vertex = 0,
    Fragment,
    Compute
};

struct ShaderTranslationOptions {
    std::string entryPoint = "main";
    std::uint32_t mslVersion = 20400;
    bool targetIOS = true;
    bool fixupClipSpace = true;
    bool flipVertexY = false;
};

struct ShaderTranslationOutput {
    std::string entryPoint = "main";
    std::vector<std::uint32_t> spirv{};
    std::string msl{};
};

struct ShaderTranslationResult {
    bool succeeded = false;
    ShaderTranslationOutput output{};
    std::string diagnostics{};
};

class ShaderTranslator {
public:
    ShaderTranslator() = default;

    ShaderTranslationResult translateToMSL(ShaderStage stage, const std::string& glslSource,
                                           const ShaderTranslationOptions& options = {}) const;
};

} // namespace metalcraft
