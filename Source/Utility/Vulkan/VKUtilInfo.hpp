//
// Created by otrush on 1/27/2024.
//

#ifndef SHIFT_UTILVULKANINFOS_HPP
#define SHIFT_UTILVULKANINFOS_HPP

#include "GLFW/glfw3.h"

#include <span>
#include <vector>
#include <optional>

namespace Shift::VK::Util {
    //! Create Info for Images View, by default no array and no mip levels.
    VkImageViewCreateInfo CreateImageViewInfo(
            VkImage image,
            VkImageViewType viewType,
            VkFormat format,
            VkImageSubresourceRange subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
            });

    //! Create info for fence, signaled by default
    [[nodiscard]] VkFenceCreateInfo CreateFenceInfo(bool isSignaled = true);

    //! Create info for semaphore, basically empty struct wrapper
    VkSemaphoreCreateInfo CreateSemaphoreInfo();

    //! TODO: Currently flags are hardcoded
    VkCommandPoolCreateInfo CreateCommandPoolInfo(uint32_t queueFamilyIndex);

    VkCommandBufferBeginInfo CreateBeginCommandBufferInfo(VkCommandBufferUsageFlags flags);

    //! Create the info for submitting jobs to queue
    VkSubmitInfo CreateSubmitInfo(
            std::span<const VkSemaphore> waitSemSpan,
            std::span<const VkSemaphore> sigSemSpan,
            std::span<const VkCommandBuffer> cmdBufSpan,
            const VkPipelineStageFlags* pipelineWaitStageMask
    );

    VkShaderModuleCreateInfo CreateShaderModuleInfo(const std::span<char>& code);

    VkPipelineVertexInputStateCreateInfo CreateInputStateInfo(const std::span<VkVertexInputAttributeDescription>& attDesc, const std::span<VkVertexInputBindingDescription>& bindDesc);

    // TODO: No args for now
    VkPipelineRasterizationStateCreateInfo CreateRasterStateInfo();

    // TODO: No args for now
    VkPipelineMultisampleStateCreateInfo CreateMultisampleStateInfo();

    // TODO: No args for now
    VkPipelineColorBlendAttachmentState CreateBlendAttachmentState();

    // TODO: No args for now
    VkPipelineColorBlendStateCreateInfo CreateBlendStateInfo(const VkPipelineColorBlendAttachmentState& att);

    // TODO: Only basic args here
    VkSamplerCreateInfo CreateSamplerInfo(
            VkFilter minFilter,
            VkFilter magFilter,
            VkSamplerMipmapMode mipFilter,
            VkSamplerAddressMode addressModeU,
            VkSamplerAddressMode addressModeV,
            VkSamplerAddressMode addressModeW,
            bool unnormalizedCoordinates = false,
            bool compareEnable = false,
            VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS,
            float mipLodBias = 0.0f,
            float minLod = 0.0f,
            float maxLod = 16.0f
    );

    // TODO: Only basic args for now
    VkPipelineDepthStencilStateCreateInfo CreateDepthStencilStateInfo();

    //! Create dynamic rendering attachment - for color attachments by default
    VkRenderingAttachmentInfoKHR CreateRenderingAttachmentInfo(VkImageView view, VkImageLayout layout, VkClearValue val, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp);

    VkPipelineRenderingCreateInfoKHR CreatePipelineRenderingInfo(std::span<VkFormat> colorFormats, std::optional<VkFormat> depthFormat);

    //! Create Dynamic Rendering info struct
    //! \param colorFormats Color formats, if size of the span is 0, ignored
    //! \param depthFormat Depth buffer format, ignored if Undefined
    //! \param stencilFormat Stencil buffer format, ignored if Undefined
    //! \return VkPipelineRenderingCreateInfoKHR structure
    VkPipelineRenderingCreateInfoKHR CreatePipelineRenderingInfo(std::span<VkFormat> colorFormats, VkFormat depthFormat, VkFormat stencilFormat);
}// Shift::VK::Util

#endif //SHIFT_UTILVULKANINFOS_HPP
