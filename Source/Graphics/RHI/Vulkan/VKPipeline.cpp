#include "VKPipeline.hpp"

#include "Utility/Vulkan/VKUtilRHI.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace Shift::VK {
    bool Pipeline::Init(const Device *device, const PipelineDescriptor &descriptor, const std::vector<ShaderStageDesc>& shaders, std::span<VkDescriptorSetLayout> descLayouts) {
        m_device = device;
        m_desc = descriptor;

        //! Shaders
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.reserve(shaders.size());
        for (auto& shader: shaders) {
            shaderStages.push_back(shader.handle->VK_GetStageInfo());
        }

        //! Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = Util::ShiftToVKPrimitiveTopology(descriptor.topology);
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        //! Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = Util::ShiftToVKVertexConfig(descriptor.vertexConfig);

        //! Dynamic state (hardcoded for now as well)
        const std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
        dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateInfo.pDynamicStates = dynamicStates.data();

        //! Viewport state (default for now)
        VkPipelineViewportStateCreateInfo viewPortStateInfo{};
        viewPortStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewPortStateInfo.viewportCount = 1;
        viewPortStateInfo.scissorCount = 1;

        //! Raster
        VkPipelineRasterizationStateCreateInfo rasterizerInfo = Util::ShiftToVKRasterizerState(descriptor.rasterizerStateDesc);

        //! Multisample
        VkPipelineMultisampleStateCreateInfo multisampleInfo = Util::ShiftToVKMultisampleDesc(descriptor.multisampleDesc);

        //! Color blend attachments
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttInfos;
        Util::ShiftToVKColorAttachmentConfig(descriptor.colorBlendConfig, &colorBlendAttInfos);

        //! Color BlendState
        VkPipelineColorBlendStateCreateInfo coloBlendStateInfo = Util::ShiftToVKColorBlendConfig(descriptor.colorBlendConfig, colorBlendAttInfos);

        //! Depth
        VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = Util::ShiftToVKDepthStencilConfig(descriptor.depthStencilConfig);

        //! Dynamic Rendering info
        std::vector<VkFormat> colorAttachments;
        colorAttachments.reserve(descriptor.colorBlendConfig.attachments.size());
        for (const auto& att: descriptor.colorBlendConfig.attachments) {
            colorAttachments.push_back(Util::ShiftToVKTextureFormat(att.format));
        }
        VkPipelineRenderingCreateInfoKHR dynamicRenderingInfo =
                Util::CreatePipelineRenderingInfo(colorAttachments,
                                                      Util::ShiftToVKTextureFormat(descriptor.depthStencilConfig.depthFormat),
                                                      Util::ShiftToVKTextureFormat(descriptor.depthStencilConfig.stencilFormat)
                                                  );

        //! Pipeline Layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descLayouts.size());
        pipelineLayoutInfo.pSetLayouts = descLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        m_layout = m_device->CreatePipelineLayout(pipelineLayoutInfo);
        if ( !(VkNullCheck(m_layout)) ) {
            return false;
        }

        //! Pipeline itself
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &dynamicRenderingInfo;
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        // Fixed function stage
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineInfo.pViewportState = &viewPortStateInfo;
        pipelineInfo.pRasterizationState = &rasterizerInfo;
        pipelineInfo.pMultisampleState = &multisampleInfo;
        pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
        pipelineInfo.pColorBlendState = &coloBlendStateInfo;
        pipelineInfo.pDynamicState = &dynamicStateInfo;

        pipelineInfo.layout = m_layout;
        pipelineInfo.renderPass = VK_NULL_HANDLE;
        pipelineInfo.subpass = 0;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        m_pipeline = m_device->CreateGraphicsPipeline(pipelineInfo);

        return VkNullCheck(m_pipeline);
    }

    //! Destroys pipeline and layout
    void Pipeline::Destroy() {
        m_device->DestroyPipeline(m_pipeline);
        m_device->DestroyPipelineLayout(m_layout);
    }
} // shift