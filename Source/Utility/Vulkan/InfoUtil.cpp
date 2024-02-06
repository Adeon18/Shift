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
    } // info
} // sft
