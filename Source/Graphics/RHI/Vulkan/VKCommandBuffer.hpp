#ifndef SHIFT_VKCOMMANDBUFFER_HPP
#define SHIFT_VKCOMMANDBUFFER_HPP

#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "VKDevice.hpp"
#include "VKFence.hpp"
#include "Assistants/VKGPUProfiler.hpp"

#include "../Common/CommandBuffer.hpp"
#include "../Common/RenderPass.hpp"
#include "../Common/GPUProfiling.hpp"

namespace Shift::VK {

    class CommandPool {
    public:
        CommandPool(const Device *device, EPoolQueueType type);
        CommandPool(const CommandPool&) = delete;
        CommandPool& operator=(const CommandPool&) = delete;

        void Reset() const;

        [[nodiscard]] VkCommandPool GetPool() const { return m_commandPool; }
        [[nodiscard]] EPoolQueueType GetType() const { return m_type; }
        [[nodiscard]] bool IsValid() const { return m_commandPool != VK_NULL_HANDLE; }

        ~CommandPool();

    private:
        const Device *m_device = nullptr;

        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        EPoolQueueType m_type;
    };
    ASSERT_INTERFACE(ICommandPool, CommandPool);

    //! A data struct that transfers information about the resource state for syncing queue ownership handoff
    struct QueueOwnershipHandoff {
        Texture* texture = nullptr;
        uint32_t srcFamily = 0;
        uint32_t dstFamily = 0;
        VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_2_NONE;
    };

    class CommandBuffer {
        friend VK::ImGuiBackend;
    public:
        CommandBuffer(const Device* device, const Instance* ins, const CommandPool& commandPool, bool isSecondary);
        CommandBuffer(const CommandBuffer&) = delete;
        CommandBuffer& operator=(const CommandBuffer&) = delete;


        [[nodiscard]] bool IsValid() const { return m_buffer != VK_NULL_HANDLE; }

        ///! ------------------- Basic Buffer Commands ------------------- !///

        //! Begin command buffer only for single time submit
        //bool BeginCommandBufferSingleTime() const;

        //! Begins command buffer that can be used multiple times
        //bool BeginCommandBuffer(VkCommandBufferUsageFlags flags = 0) const;

        //! NOTE on const-ness: methods that mutate the CPU-side recording state are
        //! non const, methods that only append to the GPU command stream stay const.

        //! Begin command buffer. Drops the per-recording texture-state table
        //! \return true if successful, false otherwise
        [[nodiscard]] bool Begin();

        [[nodiscard]] bool BeginSecondary(const SecondaryBufferBeginPayload& payload);

        //! Begin command buffer
        //! \return true if successful, false otherwise
        [[nodiscard]] bool End() const;

        //! Reset the entire buffer (same as ResetFence for now). Drops the per-recording texture-state table
        void Reset();

        void SetDebugName(const char* name) const;

        //! Debug-utils label region, must be paired. Zeroed color = tool default. When timed, also
        //! opens a GPU timing range tinted with color - should be the default way to time stuff
        void PushDebugGroup(const char* label, const DebugLabelColor& color = {}, bool timed = false);
        void PopDebugGroup();
        //! Drop a single point label into the command stream
        void InsertDebugLabel(const char* label, const DebugLabelColor& color = {}) const;

        //! Open/close a nestable GPU timing range WITHOUT a debug label Must be
        //! paired. No-op on non-graphics/secondary buffers. Mutates CPU-side range tracking, hence non-const
        void PushTimeRange(const char* name);
        void PopTimeRange();
        //! Resolve the previous recording's range timings. Caller guarantees the GPU is done
        //! with this buffer
        [[nodiscard]] std::vector<GPUTimeRange> CollectTimeRanges();

        //! Begin dynamic rendering. Builds the VkRenderingInfo from the agnostic descriptor +
        //! attachment textures
        void BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture) const;

        //! End dynamic rendering.
        void EndRenderPass() const;

        //! Record an image-layout transition. Source layout/stage come from this command buffer's
        //! per-recording view of the texture (seeded from its last-submitted state on first touch)
        //! the texture itself is only updated when this command buffer is submitted
        //! Secondary CBs are draw only so this is a primary CB only call
        //! Oh and nothing happens if new and old layouts are the same
        void TransitionTexture(Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStageFlags);

        [[nodiscard]] EPoolQueueType GetPoolType() const { return m_poolType; }

        //! Release the ownership of the resource from the current queue
        //! Just a texture transition if queue families are the same
        //! Otherwise does the release half, and logs the dst aquire half so RHI could poll it in a global buffer
        //! and other queues could do acquire barriers from this logged data
        void ReleaseQueueOwnership(Texture& texture, EPoolQueueType dstQueue, EResourceLayout dstLayout, EPipelineStageFlags dstStage);

        //! Record the acquire half from the data that ReleaseQueueOwnership logged on the other queue
        void AcquireQueueOwnership(const QueueOwnershipHandoff& handoff);

        //! Take the queue ownership transfer handoffs for this recording.
        //! Take and not Get because we are doing std::move
        [[nodiscard]] std::vector<QueueOwnershipHandoff> TakeRecordedHandoffs();

        void ExecuteSecondaryBuffers(std::span<CommandBuffer*> secondaryBuffs) const;

        //! Submit the recording. On success, commits the per-recording texture states back to the
        //! textures
        [[nodiscard]] bool Submit(
            std::span<TimelineSemaphore*> waitSems,
            std::span<uint64_t> waitVals,
            std::span<TimelineSemaphore*> signalSems,
            std::span<uint64_t> sigVals
        );

        [[nodiscard]] bool Submit(
            std::span<TimelineSemaphore*> waitTimeSems,
            std::span<uint64_t> waitVals,
            std::span<TimelineSemaphore*> signalTimeSems,
            std::span<uint64_t> sigVals,
            std::span<BinarySemaphore*> waitBinSems,
            std::span<BinarySemaphore*> signalBinSems
        );

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

        //! Update the pipeline's push-constant block.
        //! \param pipeline pipeline whose layout declares the range
        //! \param data pointer to at least size bytes
        //! \param size bytes to write
        //! \param offset byte offset RELATIVE to the start of the declared range
        void SetPushConstants(const Pipeline& pipeline, const void* data, uint32_t size, uint32_t offset) const;

        //! Bind one resource set at a set index of the pipeline's layout.
        //! Graphics bind point only as there is no compute pipeline type to take yet
        //! \param pipeline pipeline whose layout declares this set index
        //! \param setIdx index of the set in that layout (set 0 is the global bindless set)
        //! \param set the set to bind
        void BindResourceSet(const Pipeline& pipeline, uint32_t setIdx, const ResourceSet& set) const;

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

        //! Blit the texture into the other texture. Each side names one mip level and the layout
        //! that level is currently in
        //! \param srcTexture source texture + its current layout
        //! \param dstTexture destination texture + its current layout
        //! \param blitRegion blit operation description
        //! \param filter blit filter
        void BlitTexture(const TextureBlitData& srcTexture, const TextureBlitData& dstTexture, const TextureBlitRegion& blitRegion, EFilterMode filter) const;

        //! Generate mips for the shole image and transition it to final layout and final stage
        void GenerateMips(Texture& texture, EResourceLayout finalLayout, EPipelineStageFlags finalStage,
                          EFilterMode filter = EFilterMode::Linear);

        //! Set viewport, we don't support multiple
        //! \param viewport Viewport struct
        void SetViewport(Viewport viewport) const;

        //! Set scissor
        //! \param scissor scissor structure
        void SetScissor(Rect2D scissor) const;

        //! [VK backend only function] This is just a utility wrapper, is not meant to be used directly, but you can still use it.
        //! \param imgSpan the span of sync2 image memory barriers
        //! \param memSpan the span of sync2 global memory barriers
        //! \param bufMemSpan the span of sync2 buffer memory barriers
        //! \param flags meh, fuck this shit
        void VK_SetPipelineBarrier(std::span<VkImageMemoryBarrier2> imgSpan,
                                   std::span<VkMemoryBarrier2> memSpan,
                                   std::span<VkBufferMemoryBarrier2> bufMemSpan,
                                   VkDependencyFlags flags) const;

        //! [VK backend only function] Record a single sync2 image memory barrier
        //! \param imgBarrier fully-formed sync2 image barrier
        //! \param flags dependency flags
        void VK_SetPipelineBarrierImage(VkImageMemoryBarrier2 imgBarrier,
                                        VkDependencyFlags flags) const;

        //! [VK backend only function] Transition the image layout (sync2). Access masks are derived from the layouts.
        //! \param image
        //! \param oldLayout
        //! \param newLayout
        //! \param srcStage sync2 source stage mask
        //! \param dstStage sync2 destination stage mask
        //! \param subresourceRange
        void VK_TransferImageLayout(
                VkImage image,
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStage,
                VkPipelineStageFlags2 dstStage,
                VkImageSubresourceRange subresourceRange) const;

        //! [VK backend only function] Transition the image layout simpler version (sync2)
        //! \param image
        //! \param oldLayout
        //! \param newLayout
        //! \param srcStage sync2 source stage mask
        //! \param dstStage sync2 destination stage mask
        //! \param isDepth
        void VK_TransferImageLayout(
                VkImage image,
                VkImageLayout oldLayout,
                VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStage,
                VkPipelineStageFlags2 dstStage,
                bool isDepth = false) const;

        ~CommandBuffer();
    private:
        //! CPU-side view of a texture's (layout, stage) local to the current recording
        //! Struct tracks last texture transition state, as the texture itself only tracks last succesfully submitted satate
        struct TextureTrackedState {
            VkImageLayout layout;
            VkPipelineStageFlags2 stage;
        };

        //! Find a texture not-submitted state, or create one from the last submitted state if present
        TextureTrackedState& ResolveTextureState(Texture& texture);
        //! Read the effective state at this point of the recording, local not submitted one if exists, else
        //! last submitted one that is stored in the texture
        [[nodiscard]] TextureTrackedState PeekTextureState(const Texture& texture) const;

        //! Every mip and array layer of the texture
        [[nodiscard]] static VkImageSubresourceRange WholeImageRange(const Texture& texture);

        //! Transition texture but for separate mip levels, UNTRACKED!!!!
        //! TODO [FEATURE, DX12] Track the subresources and not resources
        void VK_TransitionMipRange(const Texture& texture, uint32_t baseLevel, uint32_t levelCount,
                                VkImageLayout oldLayout, VkImageLayout newLayout,
                                VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage) const;

        //! API SPECIFIC, backend-only (friended). DO NOT USE OUTSIDE THE VK BACKEND.
        [[nodiscard]] VkCommandBuffer VK_Get() const { return m_buffer; }
        const Device* m_device = nullptr;
        const Instance* m_ins = nullptr;

        VkCommandBuffer m_buffer = VK_NULL_HANDLE;

        //! This is gonna be up to 10 textures the most so vector is chill, trust me
        std::vector<std::pair<Texture*, TextureTrackedState>> m_textureStates;

        //! Queue ownership handoffs
        std::vector<QueueOwnershipHandoff> m_recordedHandoffs;

        //! GPU timing - VK-based ofc
        GPUProfiler m_profiler;

        //! Timed open ddebug group tracking
        std::vector<uint8_t> m_debugGroupTimed;

        EPoolQueueType m_poolType = EPoolQueueType::Graphics;
        bool m_isSecondary = false;
    };

    ASSERT_INTERFACE(ICommandBuffer, CommandBuffer);
} // Shift::VK

#endif //SHIFT_VKCOMMANDBUFFER_HPP
