#ifndef SHIFT_COMMANDBUFFER_HPP
#define SHIFT_COMMANDBUFFER_HPP

// DANGER: Yeaaaaah, so this must be removed further in development
#pragma optimize( "", off )

#include <vulkan/vulkan.h>

#include <memory>
#include <span>

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Synchronization/Fence.hpp"

namespace shift {
    namespace gfx {
        enum class POOL_TYPE {
            GRAPHICS,
            TRANSFER,
        };

        //! For now there can be buffers dedicated just to handling frames in flight, all the others are default and
        //! Can have infinite count
        enum class BUFFER_TYPE {
            DEFAULT,
            FLIGHT
        };

        class CommandBuffer {
        public:
            CommandBuffer(const Device& device, const Instance& ins, const VkCommandPool commandPool, POOL_TYPE type);

            [[nodiscard]] bool IsAvailable() const { return m_fence->Status() == VK_SUCCESS; }

            //! Begin command buffer only for single time submit
            bool BeginCommandBufferSingleTime() const;
            //! Begins command buffer that can be used multiple times
            bool BeginCommandBuffer(VkCommandBufferUsageFlags flags = 0) const;

            bool EndCommandBuffer() const;

            void CopyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size) const;
            void CopyBuffer(VkBuffer src, VkBuffer dest, const VkBufferCopy copyRegion) const;

            void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
            void CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy copyRegion) const;

            //! This is just a utility wrapper, basically is needed can be called explicitly
            void SetPipelineBarrier(const VkPipelineStageFlags srcStage,
                                    const VkPipelineStageFlags dstStage,
                                    const std::span<const VkImageMemoryBarrier> imgSpan,
                                    const std::span<const VkMemoryBarrier> memSpan,
                                    const std::span<const VkBufferMemoryBarrier> bufMemSpan,
                                    const VkDependencyFlags flags) const;

            void SetPipelineBarrierImage(
                    const VkPipelineStageFlags srcStage,
                    const VkPipelineStageFlags dstStage,
                    const VkImageMemoryBarrier imgBarrier,
                    const VkDependencyFlags flags) const;

            void TransferImageLayout(
                    VkImage image,
                    VkImageLayout oldLayout,
                    VkImageLayout newLayout,
                    VkPipelineStageFlags srcStage,
                    VkPipelineStageFlags dstStage,
                    VkImageSubresourceRange subresourceRange) const;

            void TransferImageLayout(
                    VkImage image,
                    VkImageLayout oldLayout,
                    VkImageLayout newLayout,
                    VkPipelineStageFlags srcStage,
                    VkPipelineStageFlags dstStage) const;

            void BindVertexBuffers(std::span<VkBuffer> buffers, std::span<VkDeviceSize> offsets, uint32_t firstBind) const;

            void BindIndexBuffer(VkBuffer buffers, uint32_t offset, VkIndexType idxType = VK_INDEX_TYPE_UINT16) const;

            void BindPipeline(VkPipeline pipeline, VkPipelineBindPoint bindPoint) const;

            void BeginRenderPass(const VkRenderPassBeginInfo& info, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE) const;

            void EndRenderPass() const;

            //! Dynamic rendering extennsion integration
            void BeginRendering(VkRenderingInfoKHR info) const;

            //! Dynamic rendering extension integration
            void EndRendering() const;

            //! Set viewport, we don't support multiple
            void SetViewPort(VkViewport viewport) const;

            void SetScissor(VkRect2D scissor) const;

            void BindDescriptorSets(std::span<VkDescriptorSet> descriptorSets,
                                    std::span<const std::uint32_t> dynamicOffsets,
                                    VkPipelineLayout layout,
                                    VkPipelineBindPoint bindPoint,
                                    std::uint32_t firstSet) const;

            void DrawIndexed(uint32_t indexCount,
                             uint32_t instanceCount,
                             uint32_t firstIndex,
                             int32_t vertexOffset,
                             uint32_t firstInstance) const;

            void Draw(uint32_t vertexCount,
                     uint32_t instanceCount,
                     uint32_t firstVertex,
                     uint32_t firstInstance) const;

            //! Buffer Submit Functions
            bool Submit() const;
            bool Submit(const VkSubmitInfo& info) const;

            bool SubmitAndWait() const;
            bool SubmitAndWait(const VkSubmitInfo& info) const;

            void Wait() const { m_fence->Wait(); };

            // TODO: Temporary
            VkCommandBuffer Get() const { return m_buffer; }
            const VkCommandBuffer* Ptr() const { return &m_buffer; }


            //! Reset the buffer fence so we can know that it is free
            void ResetFence() const;

            ~CommandBuffer() = default;

            CommandBuffer() = delete;
            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;
        private:
            const Device& m_device;
            const Instance& m_ins;

            VkCommandBuffer m_buffer;

            POOL_TYPE m_poolType;

            std::unique_ptr<Fence> m_fence;
        };
    } // gfx
} // shift

#endif //SHIFT_COMMANDBUFFER_HPP
