#include "CommandBuffer.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace sft {
    namespace gfx {
        CommandBuffer::CommandBuffer(const sft::gfx::Device &device, const VkCommandPool commandPool): m_device{device} {
            VkCommandBufferAllocateInfo allocInfoGraphics{};
            allocInfoGraphics.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfoGraphics.commandPool = commandPool;
            allocInfoGraphics.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Primary can be submitted to the queue, secondary can be called from primary buffers and inversely
            allocInfoGraphics.commandBufferCount = 1;

            if (int res = vkAllocateCommandBuffers(m_device.Get(), &allocInfoGraphics, &m_buffer); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkCommandPool! Code: {}", res);
                m_buffer = VK_NULL_HANDLE;
            }
        }

        void CommandBuffer::ResetFence() {
           m_fence->Reset();
        }

        const CommandBuffer &CommandBuffer::BeginCommandBufferSingleTime() const {
            return BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        }

        const CommandBuffer &CommandBuffer::BeginCommandBuffer(VkCommandBufferUsageFlags flags) const {
            auto info = info::CreateBeginCommandBufferInfo(flags);
            vkBeginCommandBuffer(m_buffer, &info);

            return *this;
        }

        const CommandBuffer &CommandBuffer::EndCommandBuffer() const {
            vkEndCommandBuffer(m_buffer);
            return *this;
        }


        const CommandBuffer &
        CommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dest, const VkBufferCopy copyRegion) const {
            vkCmdCopyBuffer(m_buffer, src, dest, 1, &copyRegion);

            return *this;
        }

        const CommandBuffer &CommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size) const {
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;

            return CopyBuffer(src, dest, copyRegion);
        }

        const CommandBuffer &
        CommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = {
                    width,
                    height,
                    1
            };

            return CopyBufferToImage(buffer, image, region);
        }

        const CommandBuffer &
        CommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy copyRegion) const {
            vkCmdCopyBufferToImage(
                    m_buffer,
                    buffer,
                    image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copyRegion
            );

            return *this;
        }

        const CommandBuffer &
        CommandBuffer::TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                           VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                           VkImageSubresourceRange subresourceRange) const {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            // This is filled only if we use it to do queue ffamily ownership transfer
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            barrier.image = image;
            barrier.subresourceRange = subresourceRange;

            // TODO: taken from https://github.com/inexorgame/vulkan-renderer
            switch (oldLayout) {
                case VK_IMAGE_LAYOUT_UNDEFINED:
                    barrier.srcAccessMask = 0;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                default:
                    spdlog::error("Unsupported source layout!");
                    break;
            }

            switch (newLayout) {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    if (barrier.srcAccessMask == 0) {
                        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    }
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                default:
                    spdlog::error("Unsupported destination layout!");
                    break;
            }

            return SetPipelineBarrierImage(srcStage, dstStage, barrier, 0);
        }

        const CommandBuffer &
        CommandBuffer::TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                           VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) const {

            VkImageSubresourceRange subresourceRange{};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            return TransferImageLayout(image, oldLayout, newLayout, srcStage, dstStage, subresourceRange);
        }

        const CommandBuffer &
        CommandBuffer::SetPipelineBarrier(const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                                          const std::span<const VkImageMemoryBarrier> imgSpan,
                                          const std::span<const VkMemoryBarrier> memSpan,
                                          const std::span<const VkBufferMemoryBarrier> bufMemSpan,
                                          const VkDependencyFlags flags) const {

            vkCmdPipelineBarrier(
                    m_buffer,
                    srcStage, dstStage,
                    flags,
                    memSpan.size(), memSpan.data(),
                    bufMemSpan.size(), bufMemSpan.data(),
                    imgSpan.size(), imgSpan.data()
            );

            return *this;
        }

        const CommandBuffer &
        CommandBuffer::SetPipelineBarrierImage(const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                                               const VkImageMemoryBarrier imgBarrier,
                                               const VkDependencyFlags flags) const {
            return SetPipelineBarrier(srcStage, dstStage, {&imgBarrier, 1}, {}, {}, flags);
        }
    } // gfx
} // sft
