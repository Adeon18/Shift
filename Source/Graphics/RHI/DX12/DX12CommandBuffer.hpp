#ifndef SHIFT_DX12COMMANDBUFFER_HPP
#define SHIFT_DX12COMMANDBUFFER_HPP

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "DX12Device.hpp"
#include "../Common/CommandBuffer.hpp"
#include "../Common/GPUProfiling.hpp"
#include "../Common/RenderPass.hpp"

namespace Shift::DX12 {
    class CommandPool {
    public:
        CommandPool(const Device* device, EPoolQueueType type);

        void Reset() const;
        [[nodiscard]] EPoolQueueType GetType() const;

        ~CommandPool();
    };

    ASSERT_INTERFACE(ICommandPool, CommandPool);

    class CommandBuffer {
    public:
        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] bool Begin();
        [[nodiscard]] bool BeginSecondary(const SecondaryBufferBeginPayload& payload);
        [[nodiscard]] bool End() const;
        void Reset();

        void SetDebugName(const char* name) const;

        void PushDebugGroup(const char* label, const DebugLabelColor& color = {}, bool timed = false);
        void PopDebugGroup();
        void InsertDebugLabel(const char* label, const DebugLabelColor& color = {}) const;

        void PushTimeRange(const char* name);
        void PopTimeRange();
        [[nodiscard]] std::vector<GPUTimeRange> CollectTimeRanges();

        void BeginRenderPass(const RenderPassDescriptor& desc,
            std::span<Texture*> colorTextures, std::span<const EResourceLayout> colorLayouts,
            std::optional<Texture*> depthTexture, EResourceLayout depthLayout) const;
        void EndRenderPass() const;

        void TextureBarrier(const Texture& texture, const MemoryDependency& dependency,
            EResourceLayout oldLayout, EResourceLayout newLayout,
            const TextureSubresourceRange& range) const;

        [[nodiscard]] EPoolQueueType GetPoolType() const;

        void ExecuteSecondaryBuffers(std::span<CommandBuffer*> secondaryBuffs) const;

        [[nodiscard]] bool Submit(
            std::span<TimelineSemaphore*> waitSems,
            std::span<uint64_t> waitVals,
            std::span<TimelineSemaphore*> signalSems,
            std::span<uint64_t> sigVals
        );

        void CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const;
        void FillBuffer(const BufferOpDescriptor& dstBuf, uint64_t size, uint32_t value) const;
        void CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const;
        void CopyTextureToBuffer(const TextureCopyDescriptor& srcTex, const BufferOpDescriptor& dstBuf) const;
        void BufferBarrier(const BufferOpDescriptor& buffer, uint64_t size, const MemoryDependency& dependency) const;

        void BindVertexBuffer(const Pipeline& pipeline, const BufferOpDescriptor& buffer, uint32_t bindIdx) const;
        void BindVertexBuffers(const Pipeline& pipeline, std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const;
        void BindIndexBuffer(const BufferOpDescriptor& buffer, EIndexSize indexSize) const;
        void BindGraphicsPipeline(const Pipeline& pipeline) const;
        void SetPushConstants(const Pipeline& pipeline, const void* data, uint32_t size, uint32_t offset) const;
        void BindResourceSet(const Pipeline& pipeline, uint32_t setIdx, const ResourceSet& set) const;

        void BindComputePipeline(const Pipeline& pipeline) const;
        void Dispatch(const DispatchConfig& dispatchConf) const;

        void DrawIndexed(const DrawIndexedConfig& drawConf) const;
        void Draw(const DrawConfig& drawConf) const;

        void GenerateMips(Texture& texture, EResourceLayout finalLayout, EPipelineStageFlags finalStage,
                          EFilterMode filter = EFilterMode::Linear);

        void SetViewport(Viewport viewport) const;
        void SetScissor(Rect2D scissor) const;

        ~CommandBuffer();
    };

    ASSERT_INTERFACE(ICommandBuffer, CommandBuffer);
} // Shift::DX12

#endif //SHIFT_DX12COMMANDBUFFER_HPP
