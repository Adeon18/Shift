#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "Utility/SlangCompiler/SlangCompiler.hpp"
#include "Utility/UtilStandard.hpp"

using namespace Shift;
using Graphics::Util::EShaderTarget;
using Graphics::Util::SlangCompiler;

namespace {
    struct ShaderManifestEntry {
        const char* relativePath;
        const char* entryPoint;
        EShaderType type;
    };

    //! Every shader the engine compiles at boot
    //! A shader that fails here fails the app, extend this whenever there is a new shader
    constexpr ShaderManifestEntry SHADER_MANIFEST[] = {
        { "Debug/TriangleVS.slang",  "mainVS", EShaderType::Vertex   },
        { "Debug/TrianglePS.slang",  "mainPS", EShaderType::Fragment },
        { "Fallback/Fallback.slang", "mainVS", EShaderType::Vertex   },
        { "Fallback/Fallback.slang", "mainPS", EShaderType::Fragment },
    };

#ifdef SHIFT_VULKAN_BACKEND
    constexpr EShaderTarget SHADER_TARGET = EShaderTarget::Vulkan_SPIRV;
#endif
}

TEST_SUITE("ShaderCompile") {

//! CPU-only (no device): the offline gate for the whole Shaders/Source tree.
TEST_CASE("every engine shader compiles to non-empty bytecode") {
    const std::string shaderSrcDir = Util::GetShiftShaderSrcDir();
    REQUIRE_MESSAGE(std::filesystem::exists(shaderSrcDir), "shader source dir missing: ", shaderSrcDir);

    SlangCompiler compiler;
    compiler.Init(std::vector<std::string>{shaderSrcDir}, SHADER_TARGET);

    for (const auto& entry : SHADER_MANIFEST) {
        CAPTURE(entry.relativePath);
        CAPTURE(entry.entryPoint);

        const auto result = compiler.Compile(
            std::filesystem::path(shaderSrcDir) / entry.relativePath, entry.entryPoint, entry.type);

        CHECK_MESSAGE(result.isValid, result.errorLog);
        CHECK_FALSE(result.data.empty());
    }

    compiler.Destroy();
}

}
