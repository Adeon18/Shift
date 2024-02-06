#include "Pipeline.hpp"

namespace sft {
    namespace gfx {

        void Pipeline::AddShaderStage(const Shader &shader) {
            m_shaderStages.push_back(shader.GetStageInfo());
        }

        void Pipeline::SetInputStateInfo(VkPipelineVertexInputStateCreateInfo info, VkPrimitiveTopology topology) {
            m_vertexInputInfo = info;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = topology;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            m_inputAssemblyInfo = inputAssembly;
        }

        void Pipeline::SetViewPortState() {
            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            m_viewPortStateInfo = viewportState;
        }

        void Pipeline::SetDynamicState(const std::span<VkDynamicState> &dynamicStates) {
            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();

            m_dynamicStateInfo = dynamicState;
        }

        void Pipeline::SetRasterizerInfo(VkPipelineRasterizationStateCreateInfo info) {
            m_rasterizerInfo = info;
        }

        void Pipeline::SetMultisampleInfo(VkPipelineMultisampleStateCreateInfo info) {
            m_multisampleInfo = info;
        }

        void Pipeline::SetBlendAttachment(VkPipelineColorBlendAttachmentState info) {
            m_coloBlendAttInfo = info;
        }

        void Pipeline::SetBlendState(VkPipelineColorBlendStateCreateInfo info) {
            m_coloBlendStateInfo = info;
        }

        bool Pipeline::BuildLayout(const std::span<VkDescriptorSetLayout> &descSetLayout) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            // You can specify mode than 1 layouts, why tho
            pipelineLayoutInfo.setLayoutCount = descSetLayout.size(); // Optional
            pipelineLayoutInfo.pSetLayouts = descSetLayout.data(); // Optional
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            m_layout = m_device.CreatePipelineLayout(pipelineLayoutInfo);
            return m_layout != VK_NULL_HANDLE;
        }

        bool Pipeline::Build(const RenderPass& pass, uint32_t subpassIdx) {
            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = m_shaderStages.size();
            pipelineInfo.pStages = m_shaderStages.data();
            // Fixed function stage
            pipelineInfo.pVertexInputState = (m_vertexInputInfo.has_value()) ? &m_vertexInputInfo.value(): nullptr;
            pipelineInfo.pInputAssemblyState = (m_inputAssemblyInfo.has_value()) ? &m_inputAssemblyInfo.value(): nullptr;
            pipelineInfo.pViewportState = (m_viewPortStateInfo.has_value()) ? &m_viewPortStateInfo.value(): nullptr;
            pipelineInfo.pRasterizationState = (m_rasterizerInfo.has_value()) ? &m_rasterizerInfo.value(): nullptr;
            pipelineInfo.pMultisampleState = (m_multisampleInfo.has_value()) ? &m_multisampleInfo.value(): nullptr;
            pipelineInfo.pDepthStencilState = nullptr;
            pipelineInfo.pColorBlendState = (m_coloBlendStateInfo.has_value()) ? &m_coloBlendStateInfo.value(): nullptr;
            pipelineInfo.pDynamicState = (m_dynamicStateInfo.has_value()) ? &m_dynamicStateInfo.value(): nullptr;

            pipelineInfo.layout = m_layout;
            pipelineInfo.renderPass = pass.Get();
            pipelineInfo.subpass = subpassIdx;

            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
            pipelineInfo.basePipelineIndex = -1; // Optional

            m_pipeline = m_device.CreateGraphicsPipeline(pipelineInfo);

            return m_pipeline != VK_NULL_HANDLE;
        }

        Pipeline::~Pipeline() {
            m_device.DestroyPipeline(m_pipeline);
            m_device.DestroyPipelineLayout(m_layout);
        }
    } // gfx
} // sft