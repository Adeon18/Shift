#ifndef SHIFT_VKSHADER_HPP
#define SHIFT_VKSHADER_HPP

#include <span>

#include "VKDevice.hpp"
#include "Graphics/RHI/Common/Shader.hpp"

#include "Utility/UtilStandard.hpp"

namespace Shift::VK {
    class Shader {
        friend VK::Pipeline;
    public:
        Shader(const Device* device, const ShaderDescriptor& des);
        Shader(const Device* device, std::span<uint8_t> bytecode, const ShaderDescriptor& des);
        Shader(const Shader&)=delete;
        Shader& operator=(const Shader&)=delete;

        //! Rebuild shader with new bytecode, used for hot-reloading
        void Rebuild(std::span<uint8_t> bytecode);

        [[nodiscard]] bool IsValid() const { return valid; }

        [[nodiscard]] EShaderType GetType() const { return m_descriptor.type; }
        [[nodiscard]] const ShaderDescriptor& GetDesc() const { return m_descriptor; }

        ~Shader();
    private:
        //! This is to be called by the VK pipeline only! Which is a friend class of the shader
        //! This is a Vulkan only function and is ONLY mean to be called by the Vulkan backend
        //! \return
        [[nodiscard]] VkPipelineShaderStageCreateInfo VK_GetStageInfo() const { return m_stageInfo; }

        void InitInternal(std::span<uint8_t> bytecode);

        const Device* m_device = nullptr;

        ShaderDescriptor m_descriptor;

        bool valid = false;
        VkShaderModule m_module = VK_NULL_HANDLE;
        VkPipelineShaderStageCreateInfo m_stageInfo{};
    };

    ASSERT_INTERFACE(IShader, Shader);
} // Shift::VK

#endif //SHIFT_VKSHADER_HPP
