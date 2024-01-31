//
// Created by otrush on 1/27/2024.
//

#ifndef SHIFT_UTILVULKANINFOS_HPP
#define SHIFT_UTILVULKANINFOS_HPP

#include "GLFW/glfw3.h"

namespace sft {
    namespace info {
        //! Create Info for Image View, by default no array and no mip levels.
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
    } // info
}// sft

#endif //SHIFT_UTILVULKANINFOS_HPP
