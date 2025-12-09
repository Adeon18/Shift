#include "VKShader.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    using namespace Shift::Util;

    void Shader::Init(const Device* device, const ShaderDescriptor& desc) {
        m_device = device;
        m_descriptor = desc;

        auto code = ReadFile(m_descriptor.path);

        m_module = m_device->CreateShaderModule(Util::CreateShaderModuleInfo(code));

        m_stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_stageInfo.module = m_module;
        //! By default, the entry is always main for spirv
        m_stageInfo.pName = "main";
        // You can set constant explicitly which alloes vulkan to optimize shader code based on the constants
        m_stageInfo.pSpecializationInfo = nullptr;

        m_stageInfo.stage = Util::ShiftToVKShaderType(desc.type);

        valid = VkNullCheck(m_module);
    }

    void Shader::Init(const Device *device, std::span<uint8_t> bytecode, const ShaderDescriptor& desc) {
        m_device = device;
        m_descriptor = desc;

        InitInternal(bytecode);
    }

    void Shader::Rebuild(std::span<uint8_t> bytecode) {
        Destroy();
        InitInternal(bytecode);
    }

    void Shader::Destroy() {
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
    }
} // Shift::VK