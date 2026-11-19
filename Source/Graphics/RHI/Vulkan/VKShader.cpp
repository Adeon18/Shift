#include "VKShader.hpp"

#include "Utility/Vulkan/VKDebugUtils.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    Shader::Shader(const Device *device, std::span<uint8_t> bytecode, const ShaderDescriptor& desc): m_device(device), m_descriptor(desc) {
        InitInternal(bytecode);
    }

    void Shader::Rebuild(std::span<uint8_t> bytecode) {
        m_device->DestroyShaderModule(m_module);
        InitInternal(bytecode);
    }

    Shader::~Shader() {
        m_device->DestroyShaderModule(m_module);
    }

    void Shader::InitInternal(std::span<uint8_t> bytecode) {
        m_module = m_device->CreateShaderModule(Util::CreateShaderModuleInfo(bytecode));

        m_stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_stageInfo.module = m_module;
        m_stageInfo.pName = "main";
        m_stageInfo.pSpecializationInfo = nullptr;

        m_stageInfo.stage = Util::ShiftToVKShaderType(m_descriptor.type);

        valid = VkNullCheck(m_module);

        if (valid) {
            //! The source path doubles as the module's debug name (re-applied on hot-reload Rebuild)
            //! TODO : Idk if this si how it should be
            Util::SetDebugName(m_device->Get(), VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(m_module), m_descriptor.path.c_str());
        }
    }
} // Shift::VK