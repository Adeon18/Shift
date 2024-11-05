//
// Created by otrush on 10/5/2024.
//

#ifndef SHIFT_SHADER_HPP
#define SHIFT_SHADER_HPP

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

    struct ShaderStageDesc {
        EShaderType Type;
        Shader& Shader;
    };
} // Shift

#endif //SHIFT_SHADER_HPP
