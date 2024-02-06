#include "InfoUtil.hpp"

namespace sft {
    namespace info {
        VkImageViewCreateInfo CreateImageViewInfo(
                VkImage image,
                VkImageViewType viewType,
                VkFormat format,
                VkImageSubresourceRange subresourceRange)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = viewType;
            viewInfo.format = format;
            viewInfo.subresourceRange = subresourceRange;

            return viewInfo;
        }

        VkFenceCreateInfo CreateFenceInfo(bool isSignaled) {
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = (isSignaled) ? VK_FENCE_CREATE_SIGNALED_BIT: 0;

            return fenceInfo;
        }

        VkSemaphoreCreateInfo CreateSemaphoreInfo() {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            return semaphoreInfo;
        }

        VkCommandPoolCreateInfo CreateCommandPoolInfo(uint32_t queueFamilyIndex) {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = queueFamilyIndex;

            return poolInfo;
        }

        VkCommandBufferBeginInfo CreateBeginCommandBufferInfo(VkCommandBufferUsageFlags flags) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = flags;

            return beginInfo;
        }

        VkSubmitInfo CreateSubmitInfo(
                std::span<const VkSemaphore> waitSemSpan,
                std::span<const VkSemaphore> sigSemSpan,
                std::span<const VkCommandBuffer> cmdBufSpan,
                const VkPipelineStageFlags* pipelineWaitStageMask
                ) {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            submitInfo.waitSemaphoreCount = waitSemSpan.size();
            submitInfo.pWaitSemaphores = waitSemSpan.data();
            submitInfo.pWaitDstStageMask = pipelineWaitStageMask;
            // Which command buffer to use
            submitInfo.commandBufferCount = cmdBufSpan.size();
            submitInfo.pCommandBuffers = cmdBufSpan.data();

            // Which semaphores to wait for after render is finished
            submitInfo.signalSemaphoreCount = sigSemSpan.size();
            submitInfo.pSignalSemaphores = sigSemSpan.data();

            return submitInfo;
        }

        VkShaderModuleCreateInfo CreateShaderModuleInfo(const std::span<char>& code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            // The pointer to bytecode is a pointer to const uint32_t
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            return createInfo;
        }

        VkPipelineVertexInputStateCreateInfo CreateInputStateInfo(const std::span<VkVertexInputAttributeDescription>& attDesc, const std::span<VkVertexInputBindingDescription>& bindDesc) {
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = bindDesc.size();
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attDesc.size());
            vertexInputInfo.pVertexBindingDescriptions = bindDesc.data();
            vertexInputInfo.pVertexAttributeDescriptions = attDesc.data();

            return vertexInputInfo;
        }

        VkPipelineRasterizationStateCreateInfo CreateRasterStateInfo() {
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;         // Clamps objects to min/max depth instead of discard
            rasterizer.rasterizerDiscardEnable = VK_FALSE;  // If enabled, the fragments will mever pass on to the framebuffer
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f; // Optional
            rasterizer.depthBiasClamp = 0.0f; // Optional
            rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

            return rasterizer;
        }

        VkPipelineMultisampleStateCreateInfo CreateMultisampleStateInfo() {
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling.minSampleShading = 1.0f; // Optional
            multisampling.pSampleMask = nullptr; // Optional
            multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
            multisampling.alphaToOneEnable = VK_FALSE; // Optional

            return multisampling;
        }

        VkPipelineColorBlendAttachmentState CreateBlendAttachmentState() {
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

            return colorBlendAttachment;
        }

        VkPipelineColorBlendStateCreateInfo CreateBlendStateInfo(VkPipelineColorBlendAttachmentState att) {
            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &att;
            colorBlending.blendConstants[0] = 0.0f; // Optional
            colorBlending.blendConstants[1] = 0.0f; // Optional
            colorBlending.blendConstants[2] = 0.0f; // Optional
            colorBlending.blendConstants[3] = 0.0f; // Optional

            return colorBlending;
        }
    } // info
} // sft
