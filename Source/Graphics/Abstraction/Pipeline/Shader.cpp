#include "Shader.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace shift {
    namespace gfx {
        bool Shader::CreateStage() {
            auto code = util::ReadFile(m_path);

            m_module = m_device.CreateShaderModule(info::CreateShaderModuleInfo(code));

            m_stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            m_stageInfo.module = m_module;
            m_stageInfo.pName = m_entry.c_str();
            // You can set constant explicitly which alloes vulkan to optimize shader code based on the constants
            m_stageInfo.pSpecializationInfo = nullptr;

            switch (m_type) {
                case Type::Vertex:
                    m_stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                    break;
                case Type::Geometry:
                    m_stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
                    break;
                case Type::Fragment:
                    m_stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                    break;
            }

            return m_module != VK_NULL_HANDLE;
        }

        Shader::~Shader() {
            m_device.DestroyShaderModule(m_module);
        }
    } // gfx
} // shift