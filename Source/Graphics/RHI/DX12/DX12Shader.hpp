#ifndef SHIFT_DX12SHADER_HPP
#define SHIFT_DX12SHADER_HPP

#include <cstdint>
#include <span>

#include "DX12Device.hpp"
#include "../Common/Shader.hpp"

namespace Shift::DX12 {
    class Shader {
    public:
        Shader(const Device* device, std::span<uint8_t> bytecode, const ShaderDescriptor& desc);

        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] EShaderType GetType() const;
        [[nodiscard]] const ShaderDescriptor& GetDesc() const;

        ~Shader();
    };

    ASSERT_INTERFACE(IShader, Shader);
} // Shift::DX12

#endif //SHIFT_DX12SHADER_HPP
