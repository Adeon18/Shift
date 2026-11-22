#include "DX12Shader.hpp"

#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    Shader::Shader(const Device* device, std::span<uint8_t> bytecode, const ShaderDescriptor& desc) { SRHI_NOT_IMPLEMENTED(); }

    bool Shader::IsValid() const { SRHI_NOT_IMPLEMENTED(); }
    EShaderType Shader::GetType() const { SRHI_NOT_IMPLEMENTED(); }
    const ShaderDescriptor& Shader::GetDesc() const { SRHI_NOT_IMPLEMENTED(); }

    Shader::~Shader() = default;
} // Shift::DX12
