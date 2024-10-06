#ifndef SHIFT_SHADER_HPP
#define SHIFT_SHADER_HPP

#include <vulkan/vulkan.h>

#include "Graphics/Abstraction/Device/Device.hpp"

#include "Utility/UtilStandard.hpp"

namespace Shift {
    namespace gfx {
        class Shader {
        public:
            enum class Type {
                Vertex,
                Geometry,
                Fragment
            };

            Shader(const Device& device, std::string path, Type type, std::string entry = "main"): m_device{device}, m_path{path}, m_entry{entry}, m_type{type} {}

            bool CreateStage();

            [[nodiscard]] Type GetType() const { return m_type; }
            [[nodiscard]] VkShaderModule GetModule() const { return m_module; }
            [[nodiscard]] VkPipelineShaderStageCreateInfo GetStageInfo() const { return m_stageInfo; }

            Shader() = delete;
            Shader(const Shader&) = delete;
            Shader& operator=(const Shader&) = delete;

            ~Shader();
        private:
            const Device& m_device;

            std::string m_path;
            std::string m_entry;
            Type m_type;

            VkShaderModule m_module = VK_NULL_HANDLE;
            VkPipelineShaderStageCreateInfo m_stageInfo{};
        };
    } // gfx
} // shift

#endif //SHIFT_SHADER_HPP
