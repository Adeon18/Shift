//
// Created by otrush on 1/27/2024.
//

#ifndef SHIFT_UTILVULKANINFOS_HPP
#define SHIFT_UTILVULKANINFOS_HPP

#include "GLFW/glfw3.h"

#include <span>
#include <vector>
#include <optional>

namespace shift {
    namespace info {
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
        VkPipelineColorBlendStateCreateInfo CreateBlendStateInfo(VkPipelineColorBlendAttachmentState att);

        // TODO: Only basic args here
        VkSamplerCreateInfo CreateSamplerInfo(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode);

        // TODO: Only basic args for now
        VkPipelineDepthStencilStateCreateInfo CreateDepthStencilStateInfo();

        //! Create dynamic rendering attachment - for color attachments by default
        VkRenderingAttachmentInfoKHR CreateRenderingAttachmentInfo(VkImageView view, bool isColor = true, VkClearValue = {{0.004f, 0.004f, 0.004f, 1.0f}});

        VkPipelineRenderingCreateInfoKHR CreatePipelineRenderingInfo(std::span<VkFormat> colorFormats, std::optional<VkFormat> depthFormat);
    } // info
}// shift

#endif //SHIFT_UTILVULKANINFOS_HPP
