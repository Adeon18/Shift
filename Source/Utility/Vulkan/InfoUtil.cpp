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
    } // info
} // sft
