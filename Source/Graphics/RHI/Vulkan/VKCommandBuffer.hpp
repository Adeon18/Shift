#ifndef SHIFT_VKCOMMANDBUFFER_HPP
#define SHIFT_VKCOMMANDBUFFER_HPP

#include <memory>
#include <span>

#include "VKDevice.hpp"
#include "VKFence.hpp"

#include "../Common/CommandBuffer.hpp"

namespace Shift::VK {
    class CommandBuffer {
    public:
        CommandBuffer() = default;

        [[nodiscard]] bool Init(const Device* device, const Instance* ins, VkCommandPool commandPool, EPoolQueueType type);

        ///! ------------------- Basic Buffer Commands ------------------- !///

        //! Begin command buffer only for single time submit
        //bool BeginCommandBufferSingleTime() const;

        //! Begins command buffer that can be used multiple times
        //bool BeginCommandBuffer(VkCommandBufferUsageFlags flags = 0) const;

        //! Begin command buffer
        //! \return true if successful, false otherwise
        [[nodiscard]] bool Begin() const;

        //! Begin command buffer
        //! \return true if successful, false otherwise
        [[nodiscard]] bool End() const;

        //! Reset the entire buffer (same as ResetFence for now)
        void Reset() const;

        //! Dynamic rendering extennsion integration, begin the RenderPass (not VkRenderPass but the adequate one)
        //! \param info VK Dynamic rendering info structure (should be ressolved at runtime from the RenderPass struct)
        void VK_BeginRenderPass(VkRenderingInfoKHR info) const;

        //! Dynamic rendering extension integration, end the RenderPass (not VkRenderPass but the adequate one)
        void VK_EndRenderPass() const;

        [[nodiscard]] bool Submit(
            std::span<TimelineSemaphore*> waitSems,
            std::span<uint64_t> waitVals,
            std::span<TimelineSemaphore*> signalSems,
            std::span<uint64_t> sigVals
        ) const;

        [[nodiscard]] bool Submit(
            std::span<TimelineSemaphore*> waitTimeSems,
            std::span<uint64_t> waitVals,
            std::span<TimelineSemaphore*> signalTimeSems,
            std::span<uint64_t> sigVals,
            std::span<BinarySemaphore*> waitBinSems,
            std::span<BinarySemaphore*> signalBinSems
        ) const;

        ///! ------------------- Copy Buffer Commands ------------------- !///

        //! Copy buffer data to another buffer
        //! \param srcBuf buffer + offset into the buffer
        //! \param dstBuf buffer + offset into the buffer
        //! \param size size to copy
        void CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const;

        //! Copy buffer data to a texture
        //! \param srcBuf buffer + offset into the buffer
        //! \param srcTex texture + size to copy + offset + subresource range
        void CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const;

        // TODO: [FEATURE]
        // void CopyTextureToBuffer(TextureCopyDescriptor srcTex, BufferOpDescriptor dstBuf, uint32_t size);
        // TODO: [FEATURE]
        // void CopyTextureToTexture(TextureCopyDescriptor srcTex, TextureCopyDescriptor dstTex);

        //void CopyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size) const;
        //void CopyBuffer(VkBuffer src, VkBuffer dest, const VkBufferCopy copyRegion) const;

        //void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
        //void CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy copyRegion) const;

        ///! ------------------- Rendering Buffer Commands ------------------- !///

        //! Bind a single vertex buffer
        //! \param buffers buffer + offset into the buffer
        //! \param bindIdx bind idx
        void BindVertexBuffer(const BufferOpDescriptor& buffer, uint32_t bindIdx) const;

        //! Bind a range of vertex buffers
        //! \param buffers span of buffers + offsets into the buffers
        //! \param firstBind bind idx for the first buffer
        void BindVertexBuffers(std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const;

        //! Bind an index buffer
        //! TODO: Add support for buffer indice sizes, current default and only type is uint16
        //! \param buffer buffer + offset into the buffer
        void BindIndexBuffer(const BufferOpDescriptor& buffer, EIndexSize indexSize) const;

        //! Bind the graphics pipeline
        //! \param pipeline The Pipeline wrapper
        void BindGraphicsPipeline(const Pipeline& pipeline) const;

        //! [VK backend only function] Expects a higher level RHI manager to fill in the API specific data
        //! \param descriptorSets range of ds
        //! \param dynamicOffsets dynamic offsets if any
        //! \param layout pipeline layout
        //! \param bindPoint graphics/compute/RT
        //! \param firstSet first set bind point
        void VK_BindDescriptorSets(std::span<VkDescriptorSet> descriptorSets,
                                std::span<const std::uint32_t> dynamicOffsets,
                                VkPipelineLayout layout,
                                VkPipelineBindPoint bindPoint,
                                std::uint32_t firstSet) const;

        //! Draw indexed/indexed isntanced
        //! \param drawConf draw configuration
        void DrawIndexed(const DrawIndexedConfig& drawConf) const;

        //! Draw/Draw instanced
        //! \param drawConf draw configuration
        void Draw(const DrawConfig& drawConf) const;


        ///! ------------------- Mics Buffer Commands ------------------- !///

        //! Blit the texture into the other texture
        //! \param srcTexture source texture with sizes and extents
        //! \param dstTexture destination texture with sizes and extents
        //! \param blitRegion blit operation description
        //! \param filter blit filter
        void BlitTexture(const TextureBlitData& srcTexture, const TextureBlitData& dstTexture, const TextureBlitRegion& blitRegion, EFilterMode filter) const;

        //! Set viewport, we don't support multiple
        //! \param viewport Viewport struct
        void SetViewport(Viewport viewport) const;

        //! Set scissor
        //! \param scissor scissor structure
        void SetScissor(Rect2D scissor) const;

        //! [VK backend only function] This is just a utility wrapper, is not meant to be used directly, but you can still use it. Sets a pipeline barrier
        //! \param srcStage source stage
        //! \param dstStage dest stage
        //! \param imgSpan the span of image memory barriers(if we have an image barrier)
        //! \param memSpan the span of memory barriers by default
        //! \param bufMemSpan the span of buffer barriers
        //! \param flags meh, fuck this shit
        void VK_SetPipelineBarrier(VkPipelineStageFlags srcStage,
                                   VkPipelineStageFlags dstStage,
                                   std::span<VkImageMemoryBarrier> imgSpan,
                                   std::span<VkMemoryBarrier> memSpan,
                                   std::span<VkBufferMemoryBarrier> bufMemSpan,
                                   VkDependencyFlags flags) const;

        //! [VK backend only function] Set image pipeline barrier at transition
        //! \param srcStage
        //! \param dstStage
        //! \param imgBarrier
        //! \param flags
        void VK_SetPipelineBarrierImage(VkPipelineStageFlags srcStage,
                                        VkPipelineStageFlags dstStage,
                                        VkImageMemoryBarrier imgBarrier,
                                        VkDependencyFlags flags) const;

        //! [VK backend only function] Transition the image layout
        //! \param image
        //! \param oldLayout
        //! \param newLayout
        //! \param srcStage
        //! \param dstStage
        //! \param subresourceRange
        void VK_TransferImageLayout(
                VkImage image,
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                VkPipelineStageFlags srcStage,
                VkPipelineStageFlags dstStage,
                VkImageSubresourceRange subresourceRange) const;

        //! [VK backend only function] Transition the image layout simpler version
        //! \param image
        //! \param oldLayout
        //! \param newLayout
        //! \param srcStage
        //! \param dstStage
        //! \param isDepth
        void VK_TransferImageLayout(
                VkImage image,
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                VkPipelineStageFlags srcStage,
                VkPipelineStageFlags dstStage,
                bool isDepth = false) const;

        // TODO: Temporary
        [[nodiscard]] VkCommandBuffer VK_Get() const { return m_buffer; }
        [[nodiscard]] const VkCommandBuffer* VK_Ptr() const { return &m_buffer; }

        void Destroy();
        ~CommandBuffer() = default;
    private:
        const Device* m_device = nullptr;
        const Instance* m_ins = nullptr;

        VkCommandBuffer m_buffer = VK_NULL_HANDLE;

        EPoolQueueType m_poolType = EPoolQueueType::Graphics;
    };

    ASSERT_INTERFACE(ICommandBuffer, CommandBuffer);
} // Shift::VK

#endif //SHIFT_VKCOMMANDBUFFER_HPP
