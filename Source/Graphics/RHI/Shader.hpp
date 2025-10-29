//
// Created by otrush on 10/5/2024.
//

#ifndef SHIFT_SHADER_HPP
#define SHIFT_SHADER_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"
#include "Shader.hpp"

namespace Shift {
    //! 1:1 with Vulkan
    enum class EShaderType: uint8_t {
        Vertex = 1 << 0,
        TesselationControl = 1 << 1,
        TesselationEvaluation = 1 << 2,
        Geometry = 1 << 3,
        Fragment = 1 << 4,
        Compute = 1 << 5,
    };

    struct ShaderDescriptor {
        EShaderType type;
        //! TODO: [FEATURE] probably not path but opcode here when I integrate slang
        const char* path;
        const char* entry;
    };

    struct ShaderStageDesc {
        EShaderType type;
        Shader* handle;
    };

    template<typename Shader>
    concept IShader =
        std::is_default_constructible_v<Shader> &&
        std::is_trivially_destructible_v<Shader> &&
    requires (Shader InputShader, const Device* DevicePtr, EShaderType type, const ShaderDescriptor& desc) {
        //! Path and entry name strings
        { InputShader.Init(DevicePtr, desc) } -> std::same_as<bool>;
        { CONCEPT_CONST_VAR(Shader, InputShader).GetType() } -> std::same_as<EShaderType>;
        { InputShader.Destroy() } -> std::same_as<void>;
    };
} // Shift

#endif //SHIFT_SHADER_HPP
