#ifndef SHIFT_PIPELINE_HPP
#define SHIFT_PIPELINE_HPP

#include <optional>
#include <span>

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Shader.hpp"
#include "Graphics/Abstraction/RenderPass/RenderPass.hpp"

namespace sft {
    namespace gfx {
        class Pipeline {
        public:
            Pipeline(const Device& device): m_device{device} {}

            //! Push the shader stage, should be called in order
            void AddShaderStage(const Shader& shader);

            //! Set vertex state and input assembly data
            void SetInputStateInfo(VkPipelineVertexInputStateCreateInfo info, VkPrimitiveTopology topology);

            //! Set entire viewport state
            void SetViewPortState();

            void SetDynamicState(const std::span<VkDynamicState>& dynamicStates);

            void SetRasterizerInfo(VkPipelineRasterizationStateCreateInfo info);

            void SetMultisampleInfo(VkPipelineMultisampleStateCreateInfo info);

            void SetDepthStencilInfo(VkPipelineDepthStencilStateCreateInfo info);

            void SetBlendAttachment(VkPipelineColorBlendAttachmentState info);

            void SetBlendState(VkPipelineColorBlendStateCreateInfo info);

            void SetDynamicRenderingInfo(VkPipelineRenderingCreateInfoKHR info);

            bool BuildLayout(const std::span<VkDescriptorSetLayout>& descSetLayout);

            bool Build(uint32_t subpassIdx = 0);

            [[nodiscard]] VkPipeline Get() const { return m_pipeline; }
            [[nodiscard]] VkPipelineLayout GetLayout() const { return m_layout; }

            ~Pipeline();

            Pipeline() = delete;
            Pipeline(const Pipeline&) = delete;
            Pipeline& operator=(const Pipeline&) = delete;
        private:
            const Device& m_device;

            VkPipeline m_pipeline = VK_NULL_HANDLE;
            VkPipelineLayout m_layout = VK_NULL_HANDLE;

            std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
            std::optional<VkPipelineVertexInputStateCreateInfo> m_vertexInputInfo;
            std::optional<VkPipelineInputAssemblyStateCreateInfo> m_inputAssemblyInfo;
            std::optional<VkViewport> m_viewPort;
            std::optional<VkRect2D> m_scissor;
            std::optional<VkPipelineDynamicStateCreateInfo> m_dynamicStateInfo;
            std::optional<VkPipelineViewportStateCreateInfo> m_viewPortStateInfo;
            std::optional<VkPipelineRasterizationStateCreateInfo> m_rasterizerInfo;
            std::optional<VkPipelineMultisampleStateCreateInfo> m_multisampleInfo;
            std::optional<VkPipelineColorBlendAttachmentState> m_coloBlendAttInfo;
            std::optional<VkPipelineColorBlendStateCreateInfo> m_coloBlendStateInfo;
            std::optional<VkPipelineDepthStencilStateCreateInfo> m_depthStencilStateInfo;
            std::optional<VkPipelineRenderingCreateInfoKHR> m_dynamicRenderingInfo;
        };
    } // gfx
} // sft

#endif //SHIFT_PIPELINE_HPP
