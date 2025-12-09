//
// Created by otrush on 11/26/2025.
//

#ifndef SHIFT_SLANGCOMPILER_HPP
#define SHIFT_SLANGCOMPILER_HPP

#include <vector>
#include <string>
#include <filesystem>

#include <slang.h>
#include <slang-com-ptr.h>

#include "Graphics/RHI/Common/Shader.hpp"


namespace Shift::Graphics::Util {

    enum class EShaderTarget {
        Undefined,
        Vulkan_SPIRV,
        DX12_DXIL
    };

    struct ShaderCompileResult {
        std::vector<uint8_t> data;
        std::vector<std::string> dependencies;
        bool isValid = false;
        std::string errorLog;
    };

    class SlangCompiler {
    public:
        void Init(const std::vector<std::string>& includePaths, EShaderTarget targetPlatform);

        [[nodiscard]] ShaderCompileResult Compile(const std::filesystem::path& filePath, const std::string& entryPoint, EShaderType type) const;

        void Destroy();

    private:
        Slang::ComPtr<slang::IGlobalSession> m_globalSession;
        EShaderTarget m_targetPlatform = EShaderTarget::Undefined;

        std::vector<const char*> m_includePaths;
    };

} // Shift::Graphics::Utils

#endif //SHIFT_SLANGCOMPILER_HPP