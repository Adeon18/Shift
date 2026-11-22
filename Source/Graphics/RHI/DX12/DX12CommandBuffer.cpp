#include "DX12CommandBuffer.hpp"

#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    CommandPool::CommandPool(const Device* device, EPoolQueueType type) { SRHI_NOT_IMPLEMENTED(); }

    void CommandPool::Reset() const { SRHI_NOT_IMPLEMENTED(); }
    EPoolQueueType CommandPool::GetType() const { SRHI_NOT_IMPLEMENTED(); }

    CommandPool::~CommandPool() = default;

    bool CommandBuffer::IsValid() const { SRHI_NOT_IMPLEMENTED(); }

    bool CommandBuffer::Begin() { SRHI_NOT_IMPLEMENTED(); }
    bool CommandBuffer::BeginSecondary(const SecondaryBufferBeginPayload& payload) { SRHI_NOT_IMPLEMENTED(); }
    bool CommandBuffer::End() const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::Reset() { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::SetDebugName(const char* name) const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::PushDebugGroup(const char* label, const DebugLabelColor& color, bool timed) { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::PopDebugGroup() { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::InsertDebugLabel(const char* label, const DebugLabelColor& color) const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::PushTimeRange(const char* name) { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::PopTimeRange() { SRHI_NOT_IMPLEMENTED(); }
    std::vector<GPUTimeRange> CommandBuffer::CollectTimeRanges() { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& desc,
        std::span<Texture*> colorTextures, std::span<const EResourceLayout> colorLayouts,
        std::optional<Texture*> depthTexture, EResourceLayout depthLayout) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::EndRenderPass() const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::TextureBarrier(const Texture& texture, const MemoryDependency& dependency,
        EResourceLayout oldLayout, EResourceLayout newLayout,
        const TextureSubresourceRange& range) const { SRHI_NOT_IMPLEMENTED(); }

    EPoolQueueType CommandBuffer::GetPoolType() const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::ExecuteSecondaryBuffers(std::span<CommandBuffer*> secondaryBuffs) const { SRHI_NOT_IMPLEMENTED(); }

    bool CommandBuffer::Submit(std::span<TimelineSemaphore*> waitSems, std::span<uint64_t> waitVals,
                               std::span<TimelineSemaphore*> signalSems, std::span<uint64_t> sigVals) { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::FillBuffer(const BufferOpDescriptor& dstBuf, uint64_t size, uint32_t value) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::CopyTextureToBuffer(const TextureCopyDescriptor& srcTex, const BufferOpDescriptor& dstBuf) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::BufferBarrier(const BufferOpDescriptor& buffer, uint64_t size, const MemoryDependency& dependency) const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::BindVertexBuffer(const Pipeline& pipeline, const BufferOpDescriptor& buffer, uint32_t bindIdx) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::BindVertexBuffers(const Pipeline& pipeline, std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::BindIndexBuffer(const BufferOpDescriptor& buffer, EIndexSize indexSize) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::BindGraphicsPipeline(const Pipeline& pipeline) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::SetPushConstants(const Pipeline& pipeline, const void* data, uint32_t size, uint32_t offset) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::BindResourceSet(const Pipeline& pipeline, uint32_t setIdx, const ResourceSet& set) const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::BindComputePipeline(const Pipeline& pipeline) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::Dispatch(const DispatchConfig& dispatchConf) const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::DrawIndexed(const DrawIndexedConfig& drawConf) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::Draw(const DrawConfig& drawConf) const { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::GenerateMips(Texture& texture, EResourceLayout finalLayout, EPipelineStageFlags finalStage, EFilterMode filter) { SRHI_NOT_IMPLEMENTED(); }

    void CommandBuffer::SetViewport(Viewport viewport) const { SRHI_NOT_IMPLEMENTED(); }
    void CommandBuffer::SetScissor(Rect2D scissor) const { SRHI_NOT_IMPLEMENTED(); }

    CommandBuffer::~CommandBuffer() = default;
} // Shift::DX12
