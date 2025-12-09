//
// Created by otrush on 10/5/2024.
//

#ifndef SHIFT_SHADER_HPP
#define SHIFT_SHADER_HPP

#include <span>
#include <string>
#include <concepts>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"

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

    static constexpr std::string ShaderTypeToString(EShaderType type) {
        switch (type) {
            case EShaderType::Vertex: return "Vertex";
            case EShaderType::TesselationControl: return "TesselationControl";
            case EShaderType::TesselationEvaluation: return "TesselationEvaluation";
            case EShaderType::Geometry: return "Geometry";
            case EShaderType::Fragment: return "Fragment";
            case EShaderType::Compute: return "Compute";
            default: return "Undefined";
        }
    }

    struct ShaderDescriptor {
        EShaderType type = EShaderType::Fragment;
        //! TODO: [FEATURE] probably not path but opcode here when I integrate slang
        std::string path = "";
        std::string entry = "main";
    };

    struct ShaderStageDesc {
        EShaderType type;
        Shader* handle;
    };

    template<typename Shader>
    concept IShader =
        std::is_default_constructible_v<Shader> &&
        std::is_destructible_v<Shader> &&
    requires (Shader InputShader, const Device* DevicePtr, EShaderType type, const ShaderDescriptor& desc, std::span<uint8_t> data) {
        //! Path and entry name string
        { InputShader.Init(DevicePtr, desc) } -> std::same_as<void>;
        { InputShader.Init(DevicePtr, data, desc) } -> std::same_as<void>;
        { InputShader.IsValid() } -> std::same_as<bool>;
        { CONCEPT_CONST_VAR(Shader, InputShader).GetType() } -> std::same_as<EShaderType>;
        { CONCEPT_CONST_VAR(Shader, InputShader).GetDesc() } -> std::same_as<const ShaderDescriptor&>;
        { InputShader.Destroy() } -> std::same_as<void>;
    };
} // Shift

#endif //SHIFT_SHADER_HPP
