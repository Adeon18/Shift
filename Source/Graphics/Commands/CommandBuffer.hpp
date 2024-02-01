#ifndef SHIFT_COMMANDBUFFER_HPP
#define SHIFT_COMMANDBUFFER_HPP

#include <vulkan/vulkan.h>

#include <memory>
#include <span>

#include "Graphics/Device/Device.hpp"
#include "Graphics/Synchronization/Fence.hpp"

namespace sft {
    namespace gfx {
        enum class POOL_TYPE {
            GRAPHICS,
            TRANSFER,
        };

        class CommandBuffer {
        public:
            CommandBuffer(const Device& device, const VkCommandPool commandPool, POOL_TYPE type);

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

            //! Buffer Submit Functions
            bool Submit() const;
            bool Submit(const VkSubmitInfo& info) const;

            bool SubmitAndWait() const;
            bool SubmitAndWait(const VkSubmitInfo& info) const;

            // TODO: Temporary
            VkCommandBuffer Get() const {return m_buffer;}
            const VkCommandBuffer* Ptr() const {return &m_buffer;}


            //! Reset the buffer fence so we can know that it is free
            void ResetFence();

            ~CommandBuffer() = default;

            CommandBuffer() = delete;
            CommandBuffer(const CommandBuffer&) = delete;
            CommandBuffer& operator=(const CommandBuffer&) = delete;
        private:
            const Device& m_device;

            VkCommandBuffer m_buffer;

            POOL_TYPE m_poolType;

            std::unique_ptr<Fence> m_fence;
        };
    } // gfx
} // sft

#endif //SHIFT_COMMANDBUFFER_HPP
