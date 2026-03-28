#include "metalcraft/ShaderTranslator.h"

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv_cross.hpp>
#include <spirv_msl.hpp>

#include <algorithm>
#include <cctype>
#include <mutex>
#include <sstream>
#include <string_view>
#include <vector>

namespace metalcraft {
namespace {

TBuiltInResource DefaultResources() {
    TBuiltInResource resources{};

    resources.maxLights = 32;
    resources.maxClipPlanes = 6;
    resources.maxTextureUnits = 32;
    resources.maxTextureCoords = 32;
    resources.maxVertexAttribs = 64;
    resources.maxVertexUniformComponents = 4096;
    resources.maxVaryingFloats = 64;
    resources.maxVertexTextureImageUnits = 32;
    resources.maxCombinedTextureImageUnits = 80;
    resources.maxTextureImageUnits = 32;
    resources.maxFragmentUniformComponents = 4096;
    resources.maxDrawBuffers = 32;
    resources.maxVertexUniformVectors = 128;
    resources.maxVaryingVectors = 8;
    resources.maxFragmentUniformVectors = 16;
    resources.maxVertexOutputVectors = 16;
    resources.maxFragmentInputVectors = 15;
    resources.minProgramTexelOffset = -8;
    resources.maxProgramTexelOffset = 7;
    resources.maxClipDistances = 8;
    resources.maxComputeWorkGroupCountX = 65535;
    resources.maxComputeWorkGroupCountY = 65535;
    resources.maxComputeWorkGroupCountZ = 65535;
    resources.maxComputeWorkGroupSizeX = 1024;
    resources.maxComputeWorkGroupSizeY = 1024;
    resources.maxComputeWorkGroupSizeZ = 64;
    resources.maxComputeUniformComponents = 1024;
    resources.maxComputeTextureImageUnits = 16;
    resources.maxComputeImageUniforms = 8;
    resources.maxComputeAtomicCounters = 8;
    resources.maxComputeAtomicCounterBuffers = 1;
    resources.maxVaryingComponents = 60;
    resources.maxVertexOutputComponents = 64;
    resources.maxGeometryInputComponents = 64;
    resources.maxGeometryOutputComponents = 128;
    resources.maxFragmentInputComponents = 128;
    resources.maxImageUnits = 8;
    resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    resources.maxCombinedShaderOutputResources = 8;
    resources.maxImageSamples = 0;
    resources.maxVertexImageUniforms = 0;
    resources.maxTessControlImageUniforms = 0;
    resources.maxTessEvaluationImageUniforms = 0;
    resources.maxGeometryImageUniforms = 0;
    resources.maxFragmentImageUniforms = 8;
    resources.maxCombinedImageUniforms = 8;
    resources.maxGeometryTextureImageUnits = 16;
    resources.maxGeometryOutputVertices = 256;
    resources.maxGeometryTotalOutputComponents = 1024;
    resources.maxGeometryUniformComponents = 1024;
    resources.maxGeometryVaryingComponents = 64;
    resources.maxTessControlInputComponents = 128;
    resources.maxTessControlOutputComponents = 128;
    resources.maxTessControlTextureImageUnits = 16;
    resources.maxTessControlUniformComponents = 1024;
    resources.maxTessControlTotalOutputComponents = 4096;
    resources.maxTessEvaluationInputComponents = 128;
    resources.maxTessEvaluationOutputComponents = 128;
    resources.maxTessEvaluationTextureImageUnits = 16;
    resources.maxTessEvaluationUniformComponents = 1024;
    resources.maxTessPatchComponents = 120;
    resources.maxPatchVertices = 32;
    resources.maxTessGenLevel = 64;
    resources.maxViewports = 16;
    resources.maxVertexAtomicCounters = 0;
    resources.maxTessControlAtomicCounters = 0;
    resources.maxTessEvaluationAtomicCounters = 0;
    resources.maxGeometryAtomicCounters = 0;
    resources.maxFragmentAtomicCounters = 8;
    resources.maxCombinedAtomicCounters = 8;
    resources.maxAtomicCounterBindings = 1;
    resources.maxVertexAtomicCounterBuffers = 0;
    resources.maxTessControlAtomicCounterBuffers = 0;
    resources.maxTessEvaluationAtomicCounterBuffers = 0;
    resources.maxGeometryAtomicCounterBuffers = 0;
    resources.maxFragmentAtomicCounterBuffers = 1;
    resources.maxCombinedAtomicCounterBuffers = 1;
    resources.maxAtomicCounterBufferSize = 16384;
    resources.maxTransformFeedbackBuffers = 4;
    resources.maxTransformFeedbackInterleavedComponents = 64;
    resources.maxCullDistances = 8;
    resources.maxCombinedClipAndCullDistances = 8;
    resources.maxSamples = 4;
    resources.maxMeshOutputVerticesNV = 256;
    resources.maxMeshOutputPrimitivesNV = 512;
    resources.maxMeshWorkGroupSizeX_NV = 32;
    resources.maxMeshWorkGroupSizeY_NV = 1;
    resources.maxMeshWorkGroupSizeZ_NV = 1;
    resources.maxTaskWorkGroupSizeX_NV = 32;
    resources.maxTaskWorkGroupSizeY_NV = 1;
    resources.maxTaskWorkGroupSizeZ_NV = 1;
    resources.maxMeshViewCountNV = 4;
    resources.maxDualSourceDrawBuffersEXT = 1;

    resources.limits.nonInductiveForLoops = 1;
    resources.limits.whileLoops = 1;
    resources.limits.doWhileLoops = 1;
    resources.limits.generalUniformIndexing = 1;
    resources.limits.generalAttributeMatrixVectorIndexing = 1;
    resources.limits.generalVaryingIndexing = 1;
    resources.limits.generalSamplerIndexing = 1;
    resources.limits.generalVariableIndexing = 1;
    resources.limits.generalConstantMatrixVectorIndexing = 1;

    return resources;
}

EShLanguage ToGlslangStage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex: return EShLangVertex;
        case ShaderStage::Fragment: return EShLangFragment;
        case ShaderStage::Compute: return EShLangCompute;
    }

    return EShLangVertex;
}

void EnsureGlslangInitialized() {
    static std::once_flag initFlag;
    std::call_once(initFlag, []() {
        glslang::InitializeProcess();
    });
}

std::string TrimLeft(std::string_view text) {
    std::size_t index = 0;
    while (index < text.size() && std::isspace(static_cast<unsigned char>(text[index])) != 0) {
        ++index;
    }
    return std::string(text.substr(index));
}

int DetectVersion(std::string_view source) {
    std::istringstream stream{std::string(source)};
    std::string line;
    while (std::getline(stream, line)) {
        const std::string trimmed = TrimLeft(line);
        if (trimmed.rfind("#version", 0) != 0) {
            continue;
        }

        std::istringstream lineStream(trimmed);
        std::string directive;
        int version = 0;
        lineStream >> directive >> version;
        return version > 0 ? version : 450;
    }

    return 450;
}

std::string WithVersionHeader(std::string source, int version) {
    std::istringstream stream(source);
    std::string line;
    while (std::getline(stream, line)) {
        if (TrimLeft(line).rfind("#version", 0) == 0) {
            return source;
        }
    }

    return "#version " + std::to_string(version) + " core\n" + source;
}

std::string JoinDiagnostics(glslang::TShader& shader, glslang::TProgram* program) {
    std::string diagnostics;

    const char* shaderLog = shader.getInfoLog();
    const char* shaderDebug = shader.getInfoDebugLog();
    if (shaderLog != nullptr && *shaderLog != '\0') {
        diagnostics += shaderLog;
    }
    if (shaderDebug != nullptr && *shaderDebug != '\0') {
        if (!diagnostics.empty()) {
            diagnostics += '\n';
        }
        diagnostics += shaderDebug;
    }

    if (program != nullptr) {
        const char* programLog = program->getInfoLog();
        const char* programDebug = program->getInfoDebugLog();
        if (programLog != nullptr && *programLog != '\0') {
            if (!diagnostics.empty()) {
                diagnostics += '\n';
            }
            diagnostics += programLog;
        }
        if (programDebug != nullptr && *programDebug != '\0') {
            if (!diagnostics.empty()) {
                diagnostics += '\n';
            }
            diagnostics += programDebug;
        }
    }

    return diagnostics;
}

} // namespace

ShaderTranslationResult ShaderTranslator::translateToMSL(
    ShaderStage stage, const std::string& glslSource,
    const ShaderTranslationOptions& options) const {
    ShaderTranslationResult result{};

    if (glslSource.empty()) {
        result.diagnostics = "GLSL source is empty.";
        return result;
    }

    EnsureGlslangInitialized();

    const EShLanguage glslangStage = ToGlslangStage(stage);
    int sourceVersion = DetectVersion(glslSource);
    std::string normalizedSource = WithVersionHeader(glslSource, sourceVersion);

    if (sourceVersion == 150 || sourceVersion == 330) {
        size_t pos = normalizedSource.find("#version 150");
        if (pos != std::string::npos) {
            normalizedSource.replace(pos, 12, "#version 300 es\n#define gl_VertexID gl_VertexID\n#define gl_InstanceID gl_InstanceID\n");
        } else {
            pos = normalizedSource.find("#version 330");
            if (pos != std::string::npos) {
                normalizedSource.replace(pos, 12, "#version 300 es\n#define gl_VertexID gl_VertexID\n#define gl_InstanceID gl_InstanceID\n");
            }
        }
        sourceVersion = 300;
    }

    const char* sourceStrings[] = {normalizedSource.c_str()};

    glslang::TShader shader(glslangStage);
    shader.setStrings(sourceStrings, 1);
    shader.setEntryPoint(options.entryPoint.c_str());
    shader.setSourceEntryPoint(options.entryPoint.c_str());
    shader.setEnvInput(glslang::EShSourceGlsl, glslangStage, glslang::EShClientOpenGL,
                       sourceVersion);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    shader.setAutoMapBindings(true);
    shader.setAutoMapLocations(true);

    const TBuiltInResource resources = DefaultResources();
    const EShMessages messages =
        static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
    if (!shader.parse(&resources, sourceVersion, false, messages)) {
        result.diagnostics = JoinDiagnostics(shader, nullptr);
        return result;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages)) {
        result.diagnostics = JoinDiagnostics(shader, &program);
        return result;
    }

    auto* intermediate = program.getIntermediate(glslangStage);
    if (intermediate == nullptr) {
        result.diagnostics = "glslang did not produce an intermediate representation.";
        return result;
    }

    glslang::SpvOptions spvOptions{};
    spvOptions.disableOptimizer = true;
    spvOptions.generateDebugInfo = false;

    std::vector<std::uint32_t> spirv;
    glslang::GlslangToSpv(*intermediate, spirv, &spvOptions);
    if (spirv.empty()) {
        result.diagnostics = "glslang produced an empty SPIR-V module.";
        return result;
    }

    try {
        spirv_cross::CompilerMSL compiler(spirv);
        spirv_cross::CompilerGLSL::Options commonOptions = compiler.get_common_options();
        commonOptions.vertex.fixup_clipspace = options.fixupClipSpace;
        commonOptions.vertex.flip_vert_y = options.flipVertexY;
        compiler.set_common_options(commonOptions);

        spirv_cross::CompilerMSL::Options mslOptions = compiler.get_msl_options();
        mslOptions.platform = options.targetIOS ? spirv_cross::CompilerMSL::Options::iOS
                                                : spirv_cross::CompilerMSL::Options::macOS;
        mslOptions.msl_version = options.mslVersion;
        mslOptions.ios_support_base_vertex_instance = true;
        compiler.set_msl_options(mslOptions);

        const auto entryPoints = compiler.get_entry_points_and_stages();
        auto matchingEntryPoint =
            std::find_if(entryPoints.begin(), entryPoints.end(), [&](const auto& entryPoint) {
                return entryPoint.name == options.entryPoint;
            });
        if (matchingEntryPoint != entryPoints.end()) {
            compiler.set_entry_point(matchingEntryPoint->name, matchingEntryPoint->execution_model);
        }

        result.output.entryPoint =
            matchingEntryPoint != entryPoints.end() ? matchingEntryPoint->name : options.entryPoint;
        result.output.spirv = std::move(spirv);
        result.output.msl = compiler.compile();
        result.succeeded = !result.output.msl.empty();
        if (!result.succeeded) {
            result.diagnostics = "SPIRV-Cross produced an empty MSL source.";
        }
    } catch (const spirv_cross::CompilerError& error) {
        result.diagnostics = error.what();
    }

    return result;
}

} // namespace metalcraft
