#include "CommandBuffer.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"
#include <iostream>

namespace shift {
    namespace gfx {
        CommandBuffer::CommandBuffer(const Device& device, const Instance& ins, const VkCommandPool commandPool, POOL_TYPE type): m_device{device}, m_ins{ins}, m_poolType{type} {
            VkCommandBufferAllocateInfo allocInfoGraphics{};
            allocInfoGraphics.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfoGraphics.commandPool = commandPool;
            allocInfoGraphics.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;  // Primary can be submitted to the queue, secondary can be called from primary buffers and inversely
            allocInfoGraphics.commandBufferCount = 1;

            if (int res = vkAllocateCommandBuffers(m_device.Get(), &allocInfoGraphics, &m_buffer); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkCommandPool! Code: {}", res);
                m_buffer = VK_NULL_HANDLE;
            }

            m_fence = std::make_unique<Fence>(m_device, false);
        }

        void CommandBuffer::ResetFence() const {
           m_fence->Reset();
        }

        bool CommandBuffer::BeginCommandBufferSingleTime() const {
            return BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        }

        bool CommandBuffer::BeginCommandBuffer(VkCommandBufferUsageFlags flags) const {
            auto info = info::CreateBeginCommandBufferInfo(flags);
            if (int res = vkBeginCommandBuffer(m_buffer, &info); res != VK_SUCCESS) {
                spdlog::error("Failed to begin command buffer! Code: {}", res);
                return false;
            }

            return true;
        }

        bool CommandBuffer::EndCommandBuffer() const {
            if (int res = vkEndCommandBuffer(m_buffer); res != VK_SUCCESS) {
                spdlog::error("Failed to end command buffer! Code: {}", res);
                return false;
            }
            return false;
        }


        void CommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dest, const VkBufferCopy copyRegion) const {
            vkCmdCopyBuffer(m_buffer, src, dest, 1, &copyRegion);
        }

        void CommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size) const {
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = 0; // Optional
            copyRegion.dstOffset = 0; // Optional
            copyRegion.size = size;

            CopyBuffer(src, dest, copyRegion);
        }

        void CommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const {
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

            CopyBufferToImage(buffer, image, region);
        }

        void CommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy copyRegion) const {
            vkCmdCopyBufferToImage(
                    m_buffer,
                    buffer,
                    image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &copyRegion
            );
        }

        void CommandBuffer::TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
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
                    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    if (barrier.srcAccessMask == 0) {
                        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
                    }
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    break;
                case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                    barrier.dstAccessMask = 0;
                    break;
                default:
                    spdlog::error("Unsupported destination layout!");
                    break;
            }

            SetPipelineBarrierImage(srcStage, dstStage, barrier, 0);
        }

        void CommandBuffer::TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                                VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, bool isDepth) const {

            VkImageSubresourceRange subresourceRange{};
            subresourceRange.aspectMask = (isDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT: VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = 0;
            subresourceRange.layerCount = 1;

            TransferImageLayout(image, oldLayout, newLayout, srcStage, dstStage, subresourceRange);
        }

        void CommandBuffer::SetPipelineBarrier(const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage,
                                               const std::span<const VkImageMemoryBarrier> imgSpan,
                                               const std::span<const VkMemoryBarrier> memSpan,
                                               const std::span<const VkBufferMemoryBarrier> bufMemSpan,
                                               const VkDependencyFlags flags) const {

            vkCmdPipelineBarrier(
                    m_buffer,
                    srcStage, dstStage,
                    flags,
                    static_cast<uint32_t>(memSpan.size()), memSpan.data(),
                    static_cast<uint32_t>(bufMemSpan.size()), bufMemSpan.data(),
                    static_cast<uint32_t>(imgSpan.size()), imgSpan.data()
            );
        }

        void CommandBuffer::SetPipelineBarrierImage(const VkPipelineStageFlags srcStage,
                                                    const VkPipelineStageFlags dstStage,
                                                    const VkImageMemoryBarrier imgBarrier,
                                                    const VkDependencyFlags flags) const {
            SetPipelineBarrier(srcStage, dstStage, {&imgBarrier, 1}, {}, {}, flags);
        }

        bool CommandBuffer::Submit() const {
            return Submit(info::CreateSubmitInfo({}, {}, {&m_buffer, 1}, 0));
        }

        bool CommandBuffer::Submit(const VkSubmitInfo &info) const {
            VkQueue submitQueue;
            switch (m_poolType) {
                case POOL_TYPE::GRAPHICS:
                    submitQueue = m_device.GetGraphicsQueue();
                    break;
                case POOL_TYPE::TRANSFER:
                    submitQueue = m_device.GetTransferQueue();
                    break;
            }

            if (int res = vkQueueSubmit(submitQueue,
                                        1,
                                        &info,
                                        m_fence->Get()); res != VK_SUCCESS) {
                spdlog::error("Failed to submit to queue! Code: {}", res);
                return false;
            }
            return true;
        }

        bool CommandBuffer::SubmitAndWait() const {
            bool res = Submit();
            m_fence->Wait();
            return res;
        }

        bool CommandBuffer::SubmitAndWait(const VkSubmitInfo &info) const {
            bool res = Submit(info);
            m_fence->Wait();
            return res;
        }

        void CommandBuffer::BindVertexBuffers(std::span<VkBuffer> buffers, std::span<VkDeviceSize> offsets,
                                              uint32_t firstBind) const {
            vkCmdBindVertexBuffers(
                    m_buffer,
                    firstBind,
                    static_cast<uint32_t>(buffers.size()),
                    buffers.data(),
                    offsets.data());
        }

        void CommandBuffer::BindIndexBuffer(VkBuffer buffer, uint32_t offset, VkIndexType idxType) const {
            vkCmdBindIndexBuffer(m_buffer, buffer, offset, idxType);
        }

        void CommandBuffer::BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint) const {
            vkCmdBindPipeline(m_buffer, bindPoint, pipeline);
        }

        void CommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo &info, VkSubpassContents contents) const {
            vkCmdBeginRenderPass(m_buffer, &info, contents);
        }

        void CommandBuffer::EndRenderPass() const {
            vkCmdEndRenderPass(m_buffer);
        }

        void CommandBuffer::SetViewPort(VkViewport viewport) const {
            vkCmdSetViewport(m_buffer, 0, 1, &viewport);
        }

        void CommandBuffer::SetScissor(VkRect2D scissor) const {
            vkCmdSetScissor(m_buffer, 0, 1, &scissor);
        }

        void CommandBuffer::BindDescriptorSets(const std::span<VkDescriptorSet> descriptorSets,
                                               const std::span<const std::uint32_t> dynamicOffsets,
                                               const VkPipelineLayout layout, const VkPipelineBindPoint bindPoint,
                                               std::uint32_t firstSet) const {
            vkCmdBindDescriptorSets(m_buffer,
                                    bindPoint,
                                    layout, firstSet,
                                    static_cast<uint32_t>(descriptorSets.size()),descriptorSets.data(),
                                    static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
        }

        void CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                        int32_t vertexOffset, uint32_t firstInstance) const {
            vkCmdDrawIndexed(m_buffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

        void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                 uint32_t firstInstance) const {
            vkCmdDraw(m_buffer, vertexCount, instanceCount, firstVertex, firstInstance);
        }

        void CommandBuffer::BeginRendering(VkRenderingInfoKHR info) const {
            m_ins.CallBeginRenderingExternal(m_buffer, info);
        }

        void CommandBuffer::EndRendering() const {
            m_ins.CallEndRenderingExternal(m_buffer);
        }

        void
        CommandBuffer::BlitImage(VkImage srcImage, VkImageLayout srcLayout, VkImage dstImage, VkImageLayout dstLayout,
                                 std::span<VkImageBlit> blitRegions, VkFilter filter) const {
            vkCmdBlitImage(m_buffer,
                           srcImage, srcLayout,
                           dstImage, dstLayout,
                           static_cast<uint32_t>(blitRegions.size()), blitRegions.data(),
                           filter);
        }

    } // gfx
} // shift
