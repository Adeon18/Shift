#include "VKCommandBuffer.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"
#include <iostream>
#include <array>
#include <winsock2.h>

#include "VKBuffer.hpp"
#include "VKPipeline.hpp"
#include "VKSemaphore.hpp"
#include "VKTexture.hpp"

namespace Shift::VK {
    bool CommandBuffer::Init(const Device* device, const Instance* ins, VkCommandPool commandPool, EPoolQueueType type) {
        m_device = device;
        m_ins = ins;
        m_poolType = type;

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        // Primary can be submitted to the queue, secondary can be called from primary buffers and inversely
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if ( VkCheck(vkAllocateCommandBuffers(m_device->Get(), &allocInfo, &m_buffer)) ) {
            Log(Error, "Failed to create VkCommandBuffer!");
            m_buffer = VK_NULL_HANDLE;
            return false;
        }

        return m_fence.Init(m_device, true);
    }

    void CommandBuffer::ResetFence() const {
       m_fence.Reset();
    }

    void CommandBuffer::Reset() const {
        ResetFence();
        vkResetCommandBuffer(m_buffer, 0);
    }


    // bool CommandBuffer::BeginCommandBufferSingleTime() const {
    //     return BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // }
    //
    bool CommandBuffer::Begin() const {
        auto info = Util::CreateBeginCommandBufferInfo(0);
        if ( VkCheckV(vkBeginCommandBuffer(m_buffer, &info), res) ) {
            Log(Error, "Failed to begin command buffer! Code: %d", static_cast<int>(res));
            return false;
        }

        return true;
    }

    bool CommandBuffer::End() const {
        if ( VkCheckV(vkEndCommandBuffer(m_buffer), res)) {
            Log(Error, "Failed to end command buffer! Code: %d", static_cast<int>(res));
            return false;
        }
        return true;
    }

    void CommandBuffer::CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcBuf.offset; // Optional
        copyRegion.dstOffset = dstBuf.offset; // Optional
        copyRegion.size = size;

        vkCmdCopyBuffer(m_buffer, srcBuf.buffer->VK_Get(), dstBuf.buffer->VK_Get(), 1, &copyRegion);
    }

    void CommandBuffer::CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {dstTex.offset.x, dstTex.offset.y, dstTex.offset.z };
        region.imageExtent = {
                dstTex.size.x, dstTex.size.y, dstTex.size.z
        };

        vkCmdCopyBufferToImage(
            m_buffer,
            srcBuf.buffer->VK_Get(),
            dstTex.texture->GetImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }

    void CommandBuffer::VK_TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
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

        // Mostly Taken from https://github.com/inexorgame/vulkan-renderer
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
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                //! Nothing to wait for
                barrier.srcAccessMask = 0;
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

        VK_SetPipelineBarrierImage(srcStage, dstStage, barrier, 0);
    }

    void CommandBuffer::VK_TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                            VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, bool isDepth) const {

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = (isDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT: VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        VK_TransferImageLayout(image, oldLayout, newLayout, srcStage, dstStage, subresourceRange);
    }

    void CommandBuffer::Destroy() {
        m_fence.Destroy();
    }

    void CommandBuffer::VK_SetPipelineBarrier(
        VkPipelineStageFlags srcStage,  VkPipelineStageFlags dstStage,
        std::span<VkImageMemoryBarrier> imgSpan,
        std::span<VkMemoryBarrier> memSpan,
        std::span<VkBufferMemoryBarrier> bufMemSpan,
        VkDependencyFlags flags) const
    {

        vkCmdPipelineBarrier(
                m_buffer,
                srcStage, dstStage,
                flags,
                static_cast<uint32_t>(memSpan.size()), memSpan.data(),
                static_cast<uint32_t>(bufMemSpan.size()), bufMemSpan.data(),
                static_cast<uint32_t>(imgSpan.size()), imgSpan.data()
        );
    }

    void CommandBuffer::VK_SetPipelineBarrierImage(
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage,
        VkImageMemoryBarrier imgBarrier,
        VkDependencyFlags flags) const
    {
        VK_SetPipelineBarrier(srcStage, dstStage, {&imgBarrier, 1}, {}, {}, flags);
    }

    bool CommandBuffer::Submit() const {
        VkSubmitInfo info = Util::CreateSubmitInfo({}, {}, {&m_buffer, 1}, 0);
        VkQueue submitQueue;
        switch (m_poolType) {
            case EPoolQueueType::Graphics:
                submitQueue = m_device->GetGraphicsQueue();
                break;
            case EPoolQueueType::Transfer:
                submitQueue = m_device->GetTransferQueue();
                break;
            default:
                Log(Error, "Invalid pool type!");
                return false;
        }

        if (int res = vkQueueSubmit(submitQueue,
                                    1,
                                    &info,
                                    m_fence.Get()); res != VK_SUCCESS) {
            Log(Error, "Failed to submit to queue! Code: {}", res);
            return false;
        }
        return true;
    }

    bool CommandBuffer::Submit(const Semaphore& waitSemaphore, const Semaphore& sigSemaphore) const {
        std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo info = Util::CreateSubmitInfo(
                std::span{waitSemaphore.Ptr(), 1},
                std::span{sigSemaphore.Ptr(), 1},
                std::span{&m_buffer, 1},
                waitStages.data()
        );
        VkQueue submitQueue;
        switch (m_poolType) {
            case EPoolQueueType::Graphics:
                submitQueue = m_device->GetGraphicsQueue();
                break;
            case EPoolQueueType::Transfer:
                submitQueue = m_device->GetTransferQueue();
                break;
            default:
                Log(Error, "Invalid pool type!");
                return false;
        }

        if (int res = vkQueueSubmit(submitQueue,
                                    1,
                                    &info,
                                    m_fence.Get()); res != VK_SUCCESS) {
            Log(Error, "Failed to submit to queue! Code: {}", res);
            return false;
        }
        return true;
    }

    bool CommandBuffer::SubmitAndWait() const {
        bool res = Submit();
        m_fence.Wait();
        return res;
    }

    bool CommandBuffer::SubmitAndWait(const Semaphore& waitSemaphore, const Semaphore& sigSemaphore) const {
        bool res = Submit(waitSemaphore, sigSemaphore);
        m_fence.Wait();
        return res;
    }

    void CommandBuffer::BindVertexBuffer(const BufferOpDescriptor& buffer, uint32_t bindIdx) const {
        std::vector<VkDeviceSize> offsets{static_cast<VkDeviceSize>(bindIdx)};
        std::vector<VkBuffer> buffers{buffer.buffer->VK_Get()};
        vkCmdBindVertexBuffers(
                m_buffer,
                bindIdx,
                1,
                buffers.data(),
                offsets.data());
    }

    void CommandBuffer::BindVertexBuffers(std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const {
        std::vector<VkBuffer> buffs;
        std::vector<VkDeviceSize> offsets;

        buffs.reserve(buffers.size());
        offsets.reserve(buffers.size());

        for (auto&[b, o] : buffers) {
            buffs.push_back(b->VK_Get());
            offsets.push_back(o);
        }

        vkCmdBindVertexBuffers(
                m_buffer,
                firstBind,
                static_cast<uint32_t>(buffers.size()),
                buffs.data(),
                offsets.data());
    }

    void CommandBuffer::BindIndexBuffer(const BufferOpDescriptor& buffer, EIndexSize indexSize) const {
        vkCmdBindIndexBuffer(m_buffer, buffer.buffer->VK_Get(), buffer.offset, Util::ShiftToVKIndexType(indexSize));
    }

    void CommandBuffer::BindGraphicsPipeline(const Pipeline& pipeline) const {
        vkCmdBindPipeline(m_buffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.VK_Get());
    }

    void CommandBuffer::VK_BeginRenderPass(VkRenderingInfoKHR info) const {
        m_ins->CallBeginRenderingExternal(m_buffer, info);
    }

    void CommandBuffer::VK_EndRenderPass() const {
        m_ins->CallEndRenderingExternal(m_buffer);
    }

    void CommandBuffer::SetViewport(Viewport viewport) const {
        VkViewport v = VkViewport{viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth};
        vkCmdSetViewport(m_buffer, 0, 1, &v);
    }

    void CommandBuffer::SetScissor(Rect2D scissor) const {
        VkRect2D s = VkRect2D{
            .offset = VkOffset2D{.x = scissor.offset.x, .y = scissor.offset.y},
            .extent = VkExtent2D{.width = scissor.extent.x, .height = scissor.extent.y}
            };
        vkCmdSetScissor(m_buffer, 0, 1, &s);
    }

    void CommandBuffer::VK_BindDescriptorSets(const std::span<VkDescriptorSet> descriptorSets,
                                           const std::span<const std::uint32_t> dynamicOffsets,
                                           const VkPipelineLayout layout, const VkPipelineBindPoint bindPoint,
                                           std::uint32_t firstSet) const {
        vkCmdBindDescriptorSets(m_buffer,
                                bindPoint,
                                layout, firstSet,
                                static_cast<uint32_t>(descriptorSets.size()),descriptorSets.data(),
                                static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
    }

    void CommandBuffer::DrawIndexed(const DrawIndexedConfig& drawConf) const {
        vkCmdDrawIndexed(m_buffer, drawConf.indexCount, drawConf.instanceCount, drawConf.firstIndex, drawConf.vertexOffset, drawConf.firstInstance);
    }

    void CommandBuffer::Draw(const DrawConfig& drawConf) const {
        vkCmdDraw(m_buffer, drawConf.vertexCount, drawConf.instanceCount, drawConf.firstVertex, drawConf.firstInstance);
    }

    void
    CommandBuffer::BlitTexture(const TextureBlitData& srcTexture, const TextureBlitData& dstTexture, const TextureBlitRegion& blitRegion, EFilterMode filter) const {

        auto ShiftToVKBlitRegion = [](const TextureBlitRegion& region) {
            VkImageBlit blit;
            blit.srcSubresource = VkImageSubresourceLayers{
                .aspectMask = Util::ShiftToVKTextureAspect(region.srcSubresource.aspect),
                .mipLevel = region.srcSubresource.levelCount,
                .baseArrayLayer = region.srcSubresource.baseArrayLayer,
                .layerCount = region.srcSubresource.layerCount
            };
            blit.dstSubresource = VkImageSubresourceLayers{
                .aspectMask = Util::ShiftToVKTextureAspect(region.destSubresource.aspect),
                .mipLevel = region.destSubresource.levelCount,
                .baseArrayLayer = region.destSubresource.baseArrayLayer,
                .layerCount = region.destSubresource.layerCount
            };

            blit.srcOffsets[0] = VkOffset3D{
                .x = region.srcOffsets[0].x,
                .y = region.srcOffsets[0].y,
                .z = region.srcOffsets[0].z
            };
            blit.srcOffsets[1] = VkOffset3D{
                .x = region.srcOffsets[1].x,
                .y = region.srcOffsets[1].y,
                .z = region.srcOffsets[1].z
            };

            blit.dstOffsets[0] = VkOffset3D{
                .x = region.dstOffsets[0].x,
                .y = region.dstOffsets[0].y,
                .z = region.dstOffsets[0].z
            };
            blit.dstOffsets[1] = VkOffset3D{
                .x = region.dstOffsets[1].x,
                .y = region.dstOffsets[1].y,
                .z = region.dstOffsets[1].z
            };

            return blit;
        };

        VkImageBlit blit = ShiftToVKBlitRegion(blitRegion);

        vkCmdBlitImage(m_buffer,
                       srcTexture.texture->GetImage(), Util::ShiftToVKResourceLayout(srcTexture.texture->GetResourceLayout()),
                       dstTexture.texture->GetImage(), Util::ShiftToVKResourceLayout(dstTexture.texture->GetResourceLayout()),
                       1, &blit,
                       Util::ShiftToVKFilterMode(filter));
    }
} // Shift::VK
