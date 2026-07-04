#include "VKPipeline.hpp"

#include "Utility/Vulkan/VKDebugUtils.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace Shift::VK {
    Pipeline::Pipeline(const Device *device, const PipelineDescriptor &descriptor, const std::vector<ShaderStageDesc>& shaders, std::span<VkDescriptorSetLayout> descLayouts):
    m_device(device), m_desc(descriptor), m_shaders(shaders), m_descLayouts(descLayouts.begin(), descLayouts.end()){
        InitInternal(true);
    }

    void Pipeline::Rebuild(bool createLayout) {
        //! Retire the current GPU handle; the RHI releases it via ReleaseRetired()
        //! once the GPU is finished, then InitInternal() installs the freshly built one.
        if (m_pipeline != VK_NULL_HANDLE) {
            m_retiredHandles.push_back(m_pipeline);
        }
        InitInternal(createLayout);
    }

    void Pipeline::ReleaseRetired() {
        if (m_retiredHandles.empty()) { return; }
        m_device->DestroyPipeline(m_retiredHandles.front());
        m_retiredHandles.erase(m_retiredHandles.begin());
    }

    //! Destroys pipeline and layout
    Pipeline::~Pipeline() {
        //! Release anything still pending deferred destruction (GPU is idle at teardown)
        for (VkPipeline retired : m_retiredHandles) {
            m_device->DestroyPipeline(retired);
        }
        m_retiredHandles.clear();
        m_device->DestroyPipeline(m_pipeline);
        m_device->DestroyPipelineLayout(m_layout);
    }

    void Pipeline::InitInternal(bool createLayout) {
        //! Shaders
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.reserve(m_shaders.size());
        for (auto& shader: m_shaders) {
            shaderStages.push_back(shader.handle->VK_GetStageInfo());
        }

        //! Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = Util::ShiftToVKPrimitiveTopology(m_desc.topology);
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        //! Vertex Input
        std::vector<VkVertexInputBindingDescription> bindDesc;
        std::vector<VkVertexInputAttributeDescription> attDesc;
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = Util::ShiftToVKVertexConfig(m_desc.vertexConfig, &bindDesc, &attDesc);

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
        VkPipelineRasterizationStateCreateInfo rasterizerInfo = Util::ShiftToVKRasterizerState(m_desc.rasterizerStateDesc);

        //! Multisample
        VkPipelineMultisampleStateCreateInfo multisampleInfo = Util::ShiftToVKMultisampleDesc(m_desc.multisampleDesc);

        //! Color blend attachments
        std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttInfos;
        Util::ShiftToVKColorAttachmentConfig(m_desc.colorBlendConfig, &colorBlendAttInfos);

        //! Color BlendState
        VkPipelineColorBlendStateCreateInfo coloBlendStateInfo = Util::ShiftToVKColorBlendConfig(m_desc.colorBlendConfig, colorBlendAttInfos);

        //! Depth
        VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = Util::ShiftToVKDepthStencilConfig(m_desc.depthStencilConfig);

        //! Dynamic Rendering info
        std::vector<VkFormat> colorAttachments;
        colorAttachments.reserve(m_desc.colorBlendConfig.attachments.size());
        for (const auto& att: m_desc.colorBlendConfig.attachments) {
            colorAttachments.push_back(Util::ShiftToVKTextureFormat(att.format));
        }
        VkPipelineRenderingCreateInfoKHR dynamicRenderingInfo =
                Util::CreatePipelineRenderingInfo(colorAttachments,
                                                      Util::ShiftToVKTextureFormat(m_desc.depthStencilConfig.depthFormat),
                                                      Util::ShiftToVKTextureFormat(m_desc.depthStencilConfig.stencilFormat)
                                                  );

        //! Pipeline Layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descLayouts.size());
        pipelineLayoutInfo.pSetLayouts = m_descLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (createLayout) {
            m_layout = m_device->CreatePipelineLayout(pipelineLayoutInfo);
            if ( !(VkNullCheck(m_layout)) ) {
                m_valid = false;
                return;
            }
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

        m_valid = VkNullCheck(m_pipeline);

        if (m_valid) {
            //! Re-applied on every Rebuild
            Util::SetDebugName(m_device->Get(), VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(m_pipeline), m_desc.name.c_str());
            if (createLayout) {
                Util::SetDebugName(m_device->Get(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(m_layout), (m_desc.name + "_layout").c_str());
            }
        }
    }
} // shift