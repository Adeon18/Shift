#include "VKUtilInfo.hpp"

namespace Shift::VK::Util {
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
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemSpan.size());
        submitInfo.pWaitSemaphores = waitSemSpan.data();
        submitInfo.pWaitDstStageMask = pipelineWaitStageMask;
        // Which command buffer to use
        submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBufSpan.size());
        submitInfo.pCommandBuffers = cmdBufSpan.data();

        // Which semaphores to wait for after render is finished
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(sigSemSpan.size());
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
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindDesc.size());
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
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        return colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo CreateBlendStateInfo(const VkPipelineColorBlendAttachmentState& att) {
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

    VkSamplerCreateInfo CreateSamplerInfo(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = magFilter;
        samplerInfo.minFilter = minFilter;
        samplerInfo.addressModeU = addressMode;
        samplerInfo.addressModeV = addressMode;
        samplerInfo.addressModeW = addressMode;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        // You can either use UVs or [0, width/height] coordinates
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        // This is for PCF
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        // This is mipmapping information
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 16.0f;

        return samplerInfo;
    }

    VkPipelineDepthStencilStateCreateInfo CreateDepthStencilStateInfo() {
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        return depthStencil;
    }

    VkRenderingAttachmentInfoKHR CreateRenderingAttachmentInfo(VkImageView view, bool isColor, VkClearValue val) {
        VkRenderingAttachmentInfoKHR colorAttachmentInfo{};
        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachmentInfo.imageView = view;
        colorAttachmentInfo.imageLayout = (isColor) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentInfo.clearValue = val;

        return colorAttachmentInfo;
    }

    VkPipelineRenderingCreateInfoKHR CreatePipelineRenderingInfo(std::span<VkFormat> colorFormats, std::optional<VkFormat> depthFormat) {
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
        pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
        if (depthFormat.has_value()) {
            pipelineRenderingCreateInfo.depthAttachmentFormat = *depthFormat;
        }

        return pipelineRenderingCreateInfo;
    }

    VkPipelineRenderingCreateInfoKHR CreatePipelineRenderingInfo(std::span<VkFormat> colorFormats, VkFormat depthFormat, VkFormat stencilFormat) {
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
        pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormats.data();
        if (depthFormat != VK_FORMAT_UNDEFINED) {
            pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
        }
        if (stencilFormat != VK_FORMAT_UNDEFINED) {
            pipelineRenderingCreateInfo.stencilAttachmentFormat = stencilFormat;
        }

        return pipelineRenderingCreateInfo;
    }
} // Shift::VK::Util
