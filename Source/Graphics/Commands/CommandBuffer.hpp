#ifndef SHIFT_COMMANDBUFFER_HPP
#define SHIFT_COMMANDBUFFER_HPP

#include <vulkan/vulkan.h>

#include <memory>

#include "Graphics/Device/Device.hpp"
#include "Graphics/Synchronization/Fence.hpp"

namespace sft {
    namespace gfx {
        class CommandBuffer {
        public:
            CommandBuffer(const Device& device, const VkCommandPool commandPool);

            [[nodiscard]] bool IsAvailable() const { return m_fence->Status() == VK_SUCCESS; }

            //! Begin command buffer only for single time submit
            const CommandBuffer& BeginCommandBufferSingleTime() const;
            //! Begins command buffer that can be used multiple times
            const CommandBuffer& BeginCommandBuffer(VkCommandBufferUsageFlags flags = 0) const;

            const CommandBuffer& EndCommandBuffer() const;

            const CommandBuffer& CopyBuffer(VkBuffer src, VkBuffer dest, VkDeviceSize size) const;
            const CommandBuffer& CopyBuffer(VkBuffer src, VkBuffer dest, const VkBufferCopy copyRegion) const;

            const CommandBuffer& CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
            const CommandBuffer& CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy copyRegion) const;

            //! This is just a utility wrapper, basically is needed can be called explicitly
            const CommandBuffer &SetPipelineBarrier(const VkPipelineStageFlags srcStage,
                                                                   const VkPipelineStageFlags dstStage,
                                                                   const std::span<const VkImageMemoryBarrier> imgSpan,
                                                                   const std::span<const VkMemoryBarrier> memSpan,
                                                                   const std::span<const VkBufferMemoryBarrier> bufMemSpan,
                                                                   const VkDependencyFlags flags) const;

            const CommandBuffer &SetPipelineBarrierImage(const VkPipelineStageFlags srcStage,
                                                                   const VkPipelineStageFlags dstStage,
                                                                   const VkImageMemoryBarrier imgBarrier,
                                                                   const VkDependencyFlags flags) const;

            const CommandBuffer& TransferImageLayout(
                    VkImage image,
                    VkImageLayout oldLayout,
                    VkImageLayout newLayout,
                    VkPipelineStageFlags srcStage,
                    VkPipelineStageFlags dstStage,
                    VkImageSubresourceRange subresourceRange) const;

            const CommandBuffer& TransferImageLayout(
                    VkImage image,
                    VkImageLayout oldLayout,
                    VkImageLayout newLayout,
                    VkPipelineStageFlags srcStage,
                    VkPipelineStageFlags dstStage) const;

            //! Buffer Submit Functions
            const CommandBuffer& Submit();
            const CommandBuffer& Submit(const VkSubmitInfo& info);

            const CommandBuffer& SubmitAndWait();
            const CommandBuffer& SubmitAndWait(const VkSubmitInfo& info);



            //! Reset the buffer fence so we can know that it is free
            void ResetFence();

            ~CommandBuffer() = default;

            CommandBuffer() = delete;
            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;
        private:
            const Device& m_device;

            VkCommandBuffer m_buffer;

            std::unique_ptr<Fence> m_fence;
        };
    } // gfx
} // sft

#endif //SHIFT_COMMANDBUFFER_HPP
