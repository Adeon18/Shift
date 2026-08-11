#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>
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

    //! Minimal SPIR-V reader: enough to read the decorations off a module, no dependency needed.
    //! Layout is fixed by the spec - a 5-word header, then instructions whose first word packs
    //! (wordCount << 16 | opcode)
    constexpr uint32_t SPIRV_MAGIC = 0x07230203u;
    constexpr uint32_t SPIRV_HEADER_WORDS = 5;
    constexpr uint32_t OP_DECORATE = 71;
    constexpr uint32_t DECORATION_ARRAY_STRIDE = 6;

    //! Every ArrayStride decoration in the module, in encounter order. A pointer type used for
    //! indexed access carries this decoration, which is exactly where the layout rule shows up
    std::vector<uint32_t> CollectArrayStrides(const std::vector<uint8_t>& bytecode) {
        std::vector<uint32_t> strides;
        if (bytecode.empty()) { return strides; }
        if (bytecode.size() % sizeof(uint32_t) != 0) { return strides; }

        std::vector<uint32_t> words(bytecode.size() / sizeof(uint32_t));
        std::memcpy(words.data(), bytecode.data(), bytecode.size());

        if (words.size() < SPIRV_HEADER_WORDS) { return strides; }
        if (words[0] != SPIRV_MAGIC) { return strides; }

        for (size_t i = SPIRV_HEADER_WORDS; i < words.size();) {
            const uint32_t wordCount = words[i] >> 16;
            const uint32_t opcode = words[i] & 0xFFFFu;
            //! A zero word count would not advance and would spin here forever
            if (wordCount == 0) { break; }
            if (i + wordCount > words.size()) { break; }

            //! OpDecorate: [0] header, [1] target id, [2] decoration, [3] literal
            if (opcode == OP_DECORATE) {
                const bool isArrayStride = wordCount >= 4 && words[i + 2] == DECORATION_ARRAY_STRIDE;
                if (isArrayStride) {
                    strides.push_back(words[i + 3]);
                }
            }

            i += wordCount;
        }

        return strides;
    }
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

#ifdef SHIFT_VULKAN_BACKEND
//! CPU-only. The device enables scalarBlockLayout; this asserts the SHADER side agrees, rather
//! than trusting that setting a flag on the Slang session had the effect it advertises.
//!
//! The fixture holds one `float3 positions[4]` inside a struct, reached two ways: through a
//! pointer and through a bound StructuredBuffer. Scalar packing makes the
//! array stride 12 and the struct 48; std430 rounds them to 16 and 64.
//!
//! What was measured when this was written, by building both ways: the POINTER path emits 12/48
//! with the session flag on or off - Slang packs pointer-reachable data at natural size
//! regardless. The BOUND path emits 16/64 without the flag and 12/48 with it. So the flag is
//! what stops one struct from meaning two different things depending on how a shader reaches it,
//! and this test fails loudly if a Slang upgrade moves either path. Grows into the P4.3
//! reflection test, which checks named field offsets rather than just strides.
TEST_CASE("Slang packs buffers by scalar layout on both the pointer and bound paths") {
    const std::string fixture = Util::GetShiftRoot() + "Tests/Shaders/Fixtures/ScalarLayout.slang";
    REQUIRE_MESSAGE(std::filesystem::exists(fixture), "layout fixture missing: ", fixture);

    SlangCompiler compiler;
    compiler.Init(std::vector<std::string>{Util::GetShiftShaderSrcDir()}, SHADER_TARGET);

    const auto result = compiler.Compile(fixture, "mainVS", EShaderType::Vertex);
    REQUIRE_MESSAGE(result.isValid, result.errorLog);
    REQUIRE_FALSE(result.data.empty());

    //! Scalar packing of the fixture's two types: the float3 array element, and the struct
    //! wrapping four of them
    constexpr uint32_t SCALAR_FLOAT3_STRIDE = 12;
    constexpr uint32_t SCALAR_STREAM_STRIDE = 48;

    const std::vector<uint32_t> strides = CollectArrayStrides(result.data);
    REQUIRE_MESSAGE(!strides.empty(),
                    "no ArrayStride decoration in the module: the fixture no longer produces an "
                    "indexed buffer access, so it cannot witness the layout rule");

    bool sawFloat3Stride = false;
    for (const uint32_t stride : strides) {
        CAPTURE(stride);
        const bool isScalarPacked =
                (stride == SCALAR_FLOAT3_STRIDE) || (stride == SCALAR_STREAM_STRIDE);
        sawFloat3Stride = sawFloat3Stride || (stride == SCALAR_FLOAT3_STRIDE);

        CHECK_MESSAGE(isScalarPacked,
                      "ArrayStride ", stride, " is neither 12 nor 48: std430 rounding (16 and 64) "
                      "means scalar layout stopped reaching the shader");
    }

    CHECK_MESSAGE(sawFloat3Stride,
                  "no 12-byte stride anywhere: the float3 array is no longer being witnessed");

    compiler.Destroy();
}
#endif

}
