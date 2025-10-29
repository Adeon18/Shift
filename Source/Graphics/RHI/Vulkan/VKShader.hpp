#ifndef SHIFT_VKSHADER_HPP
#define SHIFT_VKSHADER_HPP

#include "VKDevice.hpp"
#include "Graphics/RHI/Shader.hpp"

#include "Utility/UtilStandard.hpp"

namespace Shift::VK {
    class Shader {
        friend VK::Pipeline;
    public:
        Shader() = default;
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        //! Initialize the Shader module along with the respective pipeline info
        //! \param device
        //! \param type Shader type
        //! \param path Path to the shader file
        //! \param entry Function entrypoint name
        //! \return false if failed to create module
        [[nodiscard]] bool Init(const Device* device, const ShaderDescriptor& desc);

        [[nodiscard]] EShaderType GetType() const { return m_type; }

        void Destroy();
        ~Shader() = default;
    private:
        //! This is to be called by the VK pipeline only! Which is a friend class of the shader
        //! This is a Vulkan only function and is ONLY mean to be called by the Vulkan backend
        //! \return
        [[nodiscard]] VkPipelineShaderStageCreateInfo VK_GetStageInfo() const { return m_stageInfo; }

        const Device* m_device = nullptr;

        const char* m_path = nullptr;
        const char* m_entry = nullptr;
        EShaderType m_type = EShaderType::Vertex;

        VkShaderModule m_module = VK_NULL_HANDLE;
        VkPipelineShaderStageCreateInfo m_stageInfo{};
    };

    ASSERT_INTERFACE(IShader, Shader);
} // Shift::VK

#endif //SHIFT_VKSHADER_HPP
