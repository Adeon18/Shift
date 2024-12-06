#include "VKShader.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    using namespace Shift::Util;

    bool Shader::Init(const Device* device, EShaderType type, const char* path, const char* entry) {
        m_device = device;
        m_type = type;
        m_path = path;
        m_entry = entry;

        auto code = ReadFile(m_path);

        m_module = m_device->CreateShaderModule(Util::CreateShaderModuleInfo(code));

        m_stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_stageInfo.module = m_module;
        m_stageInfo.pName = m_entry;
        // You can set constant explicitly which alloes vulkan to optimize shader code based on the constants
        m_stageInfo.pSpecializationInfo = nullptr;

        m_stageInfo.stage = Util::ShiftToVKShaderType(type);

        return m_module != VK_NULL_HANDLE;
    }

    void Shader::Destroy() {
        m_device->DestroyShaderModule(m_module);
    }
} // Shift::VK