#include <doctest/doctest.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "Graphics/Managers/GlobalResourceSet.hpp"
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
        { "Forward/ForwardVS.slang", "mainVS", EShaderType::Vertex   },
        { "Forward/ForwardPS.slang", "mainPS", EShaderType::Fragment },
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
    constexpr uint32_t OP_NAME = 5;
    constexpr uint32_t OP_DECORATE = 71;
    constexpr uint32_t DECORATION_ARRAY_STRIDE = 6;
    constexpr uint32_t DECORATION_BINDING = 33;
    constexpr uint32_t DECORATION_DESCRIPTOR_SET = 34;
    constexpr uint32_t OP_TYPE_IMAGE = 25;
    constexpr uint32_t OP_TYPE_SAMPLER = 26;
    constexpr uint32_t OP_TYPE_ARRAY = 28;
    constexpr uint32_t OP_TYPE_RUNTIME_ARRAY = 29;
    constexpr uint32_t OP_TYPE_POINTER = 32;
    constexpr uint32_t OP_VARIABLE = 59;

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

    //! Every OpName literal in the module. Slang names the type it lowers a matrix into after the
    //! storage order it chose, which is the only place that choice is visible in SPIR-V: it emits
    //! no ColMajor/RowMajor decoration because it never uses a native OpTypeMatrix member
    std::vector<std::string> CollectDebugNames(const std::vector<uint8_t>& bytecode) {
        std::vector<std::string> names;
        if (bytecode.empty()) { return names; }
        if (bytecode.size() % sizeof(uint32_t) != 0) { return names; }

        std::vector<uint32_t> words(bytecode.size() / sizeof(uint32_t));
        std::memcpy(words.data(), bytecode.data(), bytecode.size());

        if (words.size() < SPIRV_HEADER_WORDS) { return names; }
        if (words[0] != SPIRV_MAGIC) { return names; }

        for (size_t i = SPIRV_HEADER_WORDS; i < words.size();) {
            const uint32_t wordCount = words[i] >> 16;
            const uint32_t opcode = words[i] & 0xFFFFu;
            if (wordCount == 0) { break; }
            if (i + wordCount > words.size()) { break; }

            //! OpName: [0] header, [1] target id, [2..] null-terminated UTF-8 packed into words
            if (opcode == OP_NAME && wordCount > 2) {
                const char* chars = reinterpret_cast<const char*>(&words[i + 2]);
                const size_t maxBytes = (wordCount - 2) * sizeof(uint32_t);
                size_t length = 0;
                while (length < maxBytes && chars[length] != '\0') { ++length; }
                names.emplace_back(chars, length);
            }

            i += wordCount;
        }

        return names;
    }

    //! What a descriptor variable actually IS, resolved through its type chain. The binding number
    //! alone cannot catch a b0/b1 swap - both numbers still exist, they just mean the wrong thing
    enum class EDeclaredKind { Sampler, Image, Other };

    //! One shader-declared descriptor: the (set, binding) pair it was decorated with, what kind of
    //! descriptor it is, and the variable name so a test can say WHICH declaration moved
    struct DeclaredDescriptor {
        std::string name;
        uint32_t set = UINT32_MAX;
        uint32_t binding = UINT32_MAX;
        EDeclaredKind kind = EDeclaredKind::Other;
    };

    //! Every variable in the module carrying BOTH a DescriptorSet and a Binding decoration, with
    //! its kind resolved by walking OpVariable -> OpTypePointer -> (OpTypeRuntimeArray|OpTypeArray)*
    //! -> OpTypeSampler / OpTypeImage. Names come from OpName, where source-level identity survives
    std::vector<DeclaredDescriptor> CollectDeclaredDescriptors(const std::vector<uint8_t>& bytecode) {
        std::vector<DeclaredDescriptor> declared;
        if (bytecode.empty()) { return declared; }
        if (bytecode.size() % sizeof(uint32_t) != 0) { return declared; }

        std::vector<uint32_t> words(bytecode.size() / sizeof(uint32_t));
        std::memcpy(words.data(), bytecode.data(), bytecode.size());

        if (words.size() < SPIRV_HEADER_WORDS) { return declared; }
        if (words[0] != SPIRV_MAGIC) { return declared; }

        std::map<uint32_t, std::string> names;
        std::map<uint32_t, uint32_t> sets;
        std::map<uint32_t, uint32_t> bindings;
        //! Type graph, just the edges needed to walk a descriptor variable down to its leaf type
        std::map<uint32_t, uint32_t> pointeeOf;      //! OpTypePointer  -> pointed-to type
        std::map<uint32_t, uint32_t> elementOf;      //! OpType[Runtime]Array -> element type
        std::map<uint32_t, uint32_t> typeOfVariable; //! OpVariable -> its (pointer) result type
        std::map<uint32_t, EDeclaredKind> leafKind;  //! OpTypeSampler / OpTypeImage -> kind

        for (size_t i = SPIRV_HEADER_WORDS; i < words.size();) {
            const uint32_t wordCount = words[i] >> 16;
            const uint32_t opcode = words[i] & 0xFFFFu;
            if (wordCount == 0) { break; }
            if (i + wordCount > words.size()) { break; }

            if (opcode == OP_NAME && wordCount > 2) {
                const char* chars = reinterpret_cast<const char*>(&words[i + 2]);
                const size_t maxBytes = (wordCount - 2) * sizeof(uint32_t);
                size_t length = 0;
                while (length < maxBytes && chars[length] != '\0') { ++length; }
                names[words[i + 1]] = std::string(chars, length);
            }

            //! OpDecorate: [0] header, [1] target id, [2] decoration, [3] literal
            if (opcode == OP_DECORATE && wordCount >= 4) {
                if (words[i + 2] == DECORATION_DESCRIPTOR_SET) { sets[words[i + 1]] = words[i + 3]; }
                if (words[i + 2] == DECORATION_BINDING) { bindings[words[i + 1]] = words[i + 3]; }
            }

            //! Type declarations: result id is always the first operand except on OpVariable,
            //! where [1] is the result TYPE and [2] is the result id
            if (opcode == OP_TYPE_IMAGE && wordCount >= 2) { leafKind[words[i + 1]] = EDeclaredKind::Image; }
            if (opcode == OP_TYPE_SAMPLER && wordCount >= 2) { leafKind[words[i + 1]] = EDeclaredKind::Sampler; }
            if (opcode == OP_TYPE_RUNTIME_ARRAY && wordCount >= 3) { elementOf[words[i + 1]] = words[i + 2]; }
            if (opcode == OP_TYPE_ARRAY && wordCount >= 3) { elementOf[words[i + 1]] = words[i + 2]; }
            if (opcode == OP_TYPE_POINTER && wordCount >= 4) { pointeeOf[words[i + 1]] = words[i + 3]; }
            if (opcode == OP_VARIABLE && wordCount >= 4) { typeOfVariable[words[i + 2]] = words[i + 1]; }

            i += wordCount;
        }

        //! Walk pointer -> arrays -> leaf. Bounded by the map sizes so a malformed or cyclic type
        //! graph cannot spin here
        auto resolveKind = [&](uint32_t variableId) {
            const auto variableType = typeOfVariable.find(variableId);
            if (variableType == typeOfVariable.end()) { return EDeclaredKind::Other; }

            const auto pointee = pointeeOf.find(variableType->second);
            if (pointee == pointeeOf.end()) { return EDeclaredKind::Other; }

            uint32_t current = pointee->second;
            for (size_t step = 0; step <= elementOf.size(); ++step) {
                const auto leaf = leafKind.find(current);
                if (leaf != leafKind.end()) { return leaf->second; }

                const auto element = elementOf.find(current);
                if (element == elementOf.end()) { return EDeclaredKind::Other; }
                current = element->second;
            }
            return EDeclaredKind::Other;
        };

        for (const auto& [id, set] : sets) {
            const auto binding = bindings.find(id);
            if (binding == bindings.end()) { continue; }

            const auto name = names.find(id);
            declared.push_back({
                name == names.end() ? std::string{} : name->second,
                set,
                binding->second,
                resolveKind(id)
            });
        }

        return declared;
    }
#endif
}

TEST_SUITE("ShaderCompile") {

//! CPU-only (no device): the offline gate for the whole Shaders/Source tree.
TEST_CASE("every engine shader compiles to non-empty bytecode") {
    const std::string shaderSrcDir = Util::GetShiftShaderSrcDir();
    REQUIRE_MESSAGE(std::filesystem::exists(shaderSrcDir), "shader source dir missing: ", shaderSrcDir);

    SlangCompiler compiler;
    compiler.Init(std::vector<std::string>{shaderSrcDir, Util::GetShiftGPUSharedDir()}, SHADER_TARGET);

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

//! CPU-only. The global set is declared TWICE - once in C++ (GlobalResourceSet::Layout, which both
//! allocates the set and goes into every pipeline layout) and once in Slang (Lib/Bindless.slang's
//! [[vk::binding]] attributes). Nothing at runtime reconciles the two: the descriptors are typed
//! and numbered on the C++ side, and the shader just indexes whatever numbers it was compiled
//! with. Swap b0 and b1 in one of the two files and you get a sampler where an image is expected -
//! which validation may catch as a type mismatch, or may not, since a partially-bound array is
//! allowed to have nothing in the slot being read.
//!
//! So this reads the numbers back out of the compiled module and compares them with the C++
//! constants. It compiles the real engine PS rather than a fixture, so it also fails if the
//! shader stops sampling bindlessly at all.
TEST_CASE("the shader's bindless declarations sit where the global set declares them") {
    const std::string shaderSrcDir = Util::GetShiftShaderSrcDir();
    const std::string trianglePS = shaderSrcDir + "Debug/TrianglePS.slang";
    REQUIRE_MESSAGE(std::filesystem::exists(trianglePS), "triangle PS missing: ", trianglePS);

    SlangCompiler compiler;
    compiler.Init(std::vector<std::string>{shaderSrcDir, Util::GetShiftGPUSharedDir()}, SHADER_TARGET);

    const auto result = compiler.Compile(trianglePS, "mainPS", EShaderType::Fragment);
    REQUIRE_MESSAGE(result.isValid, result.errorLog);
    REQUIRE_FALSE(result.data.empty());

    const std::vector<DeclaredDescriptor> declared = CollectDeclaredDescriptors(result.data);

    REQUIRE_MESSAGE(declared.size() == 2,
                    "expected exactly the sampler array and the 2D image array to be declared, got ",
                    declared.size(), " - a shader reaching the global set through anything else is "
                    "a binding this test does not know about");

    bool sawSamplers = false;
    bool sawImages = false;
    for (const DeclaredDescriptor& descriptor : declared) {
        CAPTURE(descriptor.name);
        CAPTURE(descriptor.set);
        CAPTURE(descriptor.binding);

        //! Set 0 is the global set; nothing else exists yet, and a stray set 1 here would be a
        //! declaration no pipeline layout in the engine describes
        CHECK_MESSAGE(descriptor.set == 0u, "bindless declaration left set 0");

        const bool isSamplerArray = descriptor.binding == Shift::Graphics::GlobalResourceSet::BINDING_SAMPLERS;
        const bool isImageArray = descriptor.binding == Shift::Graphics::GlobalResourceSet::BINDING_IMAGES_2D;
        sawSamplers = sawSamplers || isSamplerArray;
        sawImages = sawImages || isImageArray;

        const bool isKnownBinding = isSamplerArray || isImageArray;
        CHECK_MESSAGE(isKnownBinding,
                      "binding number has no counterpart in GlobalResourceSet - the two halves of "
                      "the global set declaration have drifted apart");

        //! The number matching is not enough on its own: swap the two attributes in the shader and
        //! both numbers still exist, they just describe the wrong descriptor. THIS is the check
        //! that catches it, because the C++ side declares b0 SAMPLER and b1 SAMPLED_IMAGE
        if (isSamplerArray) {
            CHECK_MESSAGE(descriptor.kind == EDeclaredKind::Sampler,
                          "the shader declares something other than a sampler array at "
                          "BINDING_SAMPLERS, where the C++ layout declares VK_DESCRIPTOR_TYPE_SAMPLER");
        }
        if (isImageArray) {
            CHECK_MESSAGE(descriptor.kind == EDeclaredKind::Image,
                          "the shader declares something other than an image array at "
                          "BINDING_IMAGES_2D, where the C++ layout declares VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE");
        }
    }

    CHECK_MESSAGE(sawSamplers, "no declaration at BINDING_SAMPLERS");
    CHECK_MESSAGE(sawImages, "no declaration at BINDING_IMAGES_2D");

    compiler.Destroy();
}

//! CPU-only. The second silent-corruption axis next to scalar layout: matrix STORAGE order.
//! A float4x4 crosses the boundary as raw glm bytes, so the storage mode and the multiply order
//! written in shader code have to be a consistent PAIR. Shift's pair is ROW-major storage plus
//! the row-vector convention `mul(v, M)`. Break the pair in either direction and every transform
//! is silently transposed - with BDA there is no descriptor for validation to check against.
//!
//! What was measured when this was written: the pinned mode emits GLSL `layout(column_major)` and
//! lowers `mul(v, M)` to `M * v`, so the contiguous chunks in memory are the matrix's columns
//! exactly as glm writes them. (The opposite pair - column-major storage with `mul(M, v)` -
//! computes the same thing, which is why only consistency matters and the choice was free.)
//!
//! Slang emits NO ColMajor/RowMajor decoration - it lowers matrices into a wrapper struct rather
//! than a native matrix member - so the type's NAME is the only witness in the module, and it is
//! marked only in the column-major case (`_MatrixStorage_float4x4_ColMajornatural` vs
//! `_MatrixStorage_float4x4natural`). The assertion is therefore a negative one, and the REQUIRE
//! below is what keeps it honest: if Slang stops emitting the name, this test fails instead of
//! passing on a check that never had anything to look at.
TEST_CASE("Slang stores matrices row-major, pairing with the shaders' mul(v, M)") {
    const std::string fixture = Util::GetShiftRoot() + "Tests/Shaders/Fixtures/MatrixLayout.slang";
    REQUIRE_MESSAGE(std::filesystem::exists(fixture), "matrix layout fixture missing: ", fixture);

    SlangCompiler compiler;
    compiler.Init(std::vector<std::string>{Util::GetShiftShaderSrcDir()}, SHADER_TARGET);

    const auto result = compiler.Compile(fixture, "mainVS", EShaderType::Vertex);
    REQUIRE_MESSAGE(result.isValid, result.errorLog);
    REQUIRE_FALSE(result.data.empty());

    //! Slang marks the lowered type's name only when it picked column-major storage:
    //! _MatrixStorage_float4x4_ColMajornatural vs _MatrixStorage_float4x4natural
    constexpr const char* MATRIX_STORAGE_PREFIX = "_MatrixStorage_float4x4";
    constexpr const char* COLUMN_MAJOR_MARKER = "ColMajor";

    const std::vector<std::string> names = CollectDebugNames(result.data);

    std::vector<std::string> matrixTypeNames;
    for (const std::string& name : names) {
        if (name.find(MATRIX_STORAGE_PREFIX) != std::string::npos) {
            matrixTypeNames.push_back(name);
        }
    }

    REQUIRE_MESSAGE(!matrixTypeNames.empty(),
                    "no matrix storage type name in the module: either the fixture stopped "
                    "carrying a float4x4, or Slang stopped naming the type it lowers one into - "
                    "in which case this test needs a new witness rather than a green tick");

    for (const std::string& name : matrixTypeNames) {
        CAPTURE(name);
        const bool isRowMajor = name.find(COLUMN_MAJOR_MARKER) == std::string::npos;
        CHECK_MESSAGE(isRowMajor,
                      "matrix storage flipped to column-major while the shaders still write "
                      "mul(v, M): every transform is now transposed, and nothing else in the "
                      "stack will tell you");
    }

    compiler.Destroy();
}
#endif

}
