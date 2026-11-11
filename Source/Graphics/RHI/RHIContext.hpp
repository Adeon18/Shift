//
// Created by otrush on 11/10/2025.
//

#ifndef SHIFT_RHICONTEXT_HPP
#define SHIFT_RHICONTEXT_HPP

#include <algorithm>
#include <concepts>
#include <array>
#include <iterator>
#include <vector>

#include "Common/Capabilities.hpp"
#include "Common/Types.hpp"
#include "Common/Texture.hpp"
#include "Common/Buffer.hpp"
#include "Common/Pipeline.hpp"
#include "Common/Sampler.hpp"
#include "Common/Swapchain.hpp"
#include "Common/Shader.hpp"
#include "Common/RenderPass.hpp"
#include "Common/GPUProfiling.hpp"
#include "Config/EngineConfig.hpp"

#include "Utility/UtilStandard.hpp"
#include "Utility/Assertions.hpp"

namespace Shift {
    namespace RHI {
        struct Vulkan {
            static constexpr const char* Name = "Vulkan";
            static constexpr RHIRequiredFeatures requiredFeatures{
                //! A shader dereferencing a buffer address needs a 64-bit integer type
                .shaderInt64 = true,

                .VK_timelineSemaphore = true,
                .VK_descriptorIndexing = true,
                .VK_dynamicRendering = true,
                .VK_synchronization2 = true,
                .VK_bufferDeviceAddress = true,
                .VK_scalarBlockLayout = true,

                .VK_enableValidationLayers = true,
                .VK_requireSwapchain = true,
                .VK_hostQueryReset = true,
                .VK_presentWait = false,
                .VK_maintenance1 = true,
                //! Slang's SV_VertexID lowering reads BaseVertex to match HLSL semantics
                .VK_shaderDrawParameters = true,
            };
        };

        struct DX12 {
            static constexpr const char* Name = "DX12";
            static constexpr RHIRequiredFeatures requiredFeatures{};
        };
    } // RHI

    // For now:D
    template<typename T>
    concept ValidAPI = std::same_as<T, RHI::Vulkan>;

    //! A semaphore plus the value to wait for or signal, depending on
    //! which side of a submit it is handed to. Used in RHILocal and Global
    struct SubmitTimelinePayload {
        TimelineSemaphore* semaphore = nullptr;
        uint64_t value = 0;
    };

    //! Forward dec for further per API local constructs (device, instance, some descriptor specific stuff)
    template<ValidAPI API> struct RHILocal {};

} // Shift

#ifdef SHIFT_VULKAN_BACKEND
#include "Vulkan/VKRHI_Impl.hpp"
#include "RHILocal_VK.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"
#endif

#include "ResourceStateTracker.hpp"


namespace Shift {

    enum class EContextType { Graphics, Compute, Transfer };

    //! Which command-pool/queue family a context of this type submits to
    inline EPoolQueueType ToPoolQueueType(EContextType type) {
        switch (type) {
            case EContextType::Graphics: return EPoolQueueType::Graphics;
            case EContextType::Compute:  return EPoolQueueType::Compute;
            case EContextType::Transfer: return EPoolQueueType::Transfer;
        }
        return EPoolQueueType::Graphics;
    }

    template<ValidAPI API>
    class RHIEncoder {
    public:
        RHIEncoder() = default;
        RHIEncoder(CommandBuffer* cmd, ResourceStateTracker<API>* tracker): m_boundCB{cmd}, m_tracker{tracker} {}
        RHIEncoder(const RHIEncoder&) = default;
        RHIEncoder& operator=(const RHIEncoder&) = default;

        void BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture);

        void EndRenderPass();

        ///! ------------------- Copy Buffer Commands ------------------- !///

        //! Copy buffer data to another buffer
        //! \param srcBuf buffer + offset into the buffer
        //! \param dstBuf buffer + offset into the buffer
        //! \param size size to copy
        void CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const;

        //! Fill a buffer range with a repeated 32-bit value, no staging buffer. A transfer write
        //! \param dstBuf buffer + offset into the buffer, offset a multiple of 4
        //! \param size bytes to fill, a multiple of 4
        //! \param value the 32-bit pattern, e.g. std::bit_cast<uint32_t>(1.0f)
        void FillBuffer(const BufferOpDescriptor& dstBuf, uint64_t size, uint32_t value) const;

        //! Copy buffer data to a texture
        //! \param srcBuf buffer + offset into the buffer
        //! \param dstTex texture + size to copy + offset + subresource range
        void CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const;

        //! Copy texture to buffer
        //! \param srcTex texture + size to copy + offset + subresource range (levelCount must be 1)
        //! \param dstBuf buffer + offset into the buffer
        void CopyTextureToBuffer(const TextureCopyDescriptor& srcTex, const BufferOpDescriptor& dstBuf) const;

        ///! ------------------- Rendering Buffer Commands ------------------- !///

        //! Bind a single vertex buffer
        //! \param buffer buffer + offset into the buffer
        //! \param bindIdx bind idx
        void BindVertexBuffer(const BufferOpDescriptor& buffer, uint32_t bindIdx) const;

        //! Bind a range of vertex buffers
        //! \param buffers span of buffers + offsets into the buffers
        //! \param firstBind bind idx for the first buffer
        void BindVertexBuffers(std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const;

        //! Bind an index buffer
        //! \param buffer buffer + offset into the buffer
        void BindIndexBuffer(const BufferOpDescriptor& buffer, EIndexSize indexSize) const;

        //! Bind the graphics pipeline
        //! \param pipeline The Pipeline wrapper
        void BindGraphicsPipeline(const Pipeline& pipeline) const;

        //! Bind one resource set at a set index of the pipeline's layout
        //! \param pipeline pipeline whose layout declares this set index
        //! \param setIdx index of the set in that layout (set 0 is the global bindless set)
        //! \param set the set to bind
        void BindResourceSet(const Pipeline& pipeline, uint32_t setIdx, const ResourceSet& set) const;

        //! Write the pipeline's push-constant block.
        //! \param pipeline pipeline whose layout declares the range
        //! \param data pointer to at least size bytes
        //! \param size bytes to write
        //! \param offset byte offset RELATIVE to the start of the declared range
        void SetPushConstants(const Pipeline& pipeline, const void* data, uint32_t size, uint32_t offset = 0) const;

        void DrawIndexed(const DrawIndexedConfig& drawConf) const;

        //! Draw/Draw instanced
        //! \param drawConf draw configuration
        void Draw(const DrawConfig& drawConf) const;

        ///! ------------------- Compute Commands ------------------- !///
        //! Bind the compute pipeline
        //! \param pipeline The Pipeline wrapper
        void BindComputePipeline(const Pipeline& pipeline) const;

        //! Dispatch compute work
        //! \param dispatchConf thread group counts
        void Dispatch(const DispatchConfig& dispatchConf) const;

        //! Self explanatory, tracks nothing
        //! \param buffer buffer + offset into the buffer
        //! \param size bytes covered by the dependency
        //! \param dependency the four scopes to order
        void BufferBarrier(const BufferOpDescriptor& buffer, uint64_t size, const MemoryDependency& dependency) const;

        ///! ------------------- Mics Buffer Commands ------------------- !///

        //! Blit the texture into the other texture
        //! \param srcTexture source texture with sizes and extents
        //! \param dstTexture destination texture with sizes and extents
        //! \param blitRegion blit operation description
        //! \param filter blit filter
        void BlitTexture(const TextureBlitData& srcTexture, const TextureBlitData& dstTexture, const TextureBlitRegion& blitRegion, EFilterMode filter) const;

        //! Set viewport, we don't support multiple
        //! \param viewport Viewport struct
        void SetViewport(const Viewport& viewport) const;

        //! Set scissor
        //! \param scissor scissor structure
        void SetScissor(const Rect2D& scissor) const;

        //! Debug label region for validation output and captures, must be paired. No-op in
        //! builds without debug-utils. Portable. 0 color -> up to impl. timed=true also opens a
        //! colored GPU timing range
        void PushDebugGroup(const char* label, const DebugLabelColor& color = {}, bool timed = false) const;
        void PopDebugGroup() const;
        //! Drop a single point label into the command stream
        void InsertDebugLabel(const char* label, const DebugLabelColor& color = {}) const;

        //! Open/close a nestable GPU timing range WITHOUT a debug label
        void PushTimeRange(const char* name) const;
        void PopTimeRange() const;

        //! Non const texture because recording a transition may change the texture state down the road as it logs the ptr
        void TransitionTexture(Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStageFlags);

        //! Release ownership of this texture from the queue family
        void ReleaseQueueOwnership(Texture& texture, EContextType dstContext, EResourceLayout dstLayout, EPipelineStageFlags dstStage);

        //! Generate mips for the whole image and transition it to final layout and final stage.
        //! Nearest is for formats that cannot be filtered linearly, integer ones above all
        void GenerateMips(Texture& texture, EResourceLayout finalLayout, EPipelineStageFlags finalStage,
                          EFilterMode filter = EFilterMode::Linear);
    private:
        CommandBuffer* m_boundCB = nullptr;
        ResourceStateTracker<API>* m_tracker = nullptr;
    };

    template<ValidAPI API>
    class RHIContext {
    public:
        RHIContext() = default;

        // Init for primary or secondary context
        bool Init(RHILocal<API>* local, EContextType type, bool secondary = false);
        void Destroy();

        [[nodiscard]] CommandBuffer& GetCommandBuffer() {return *m_cmdBuffer;}

        [[nodiscard]] bool BeginCmds();

        [[nodiscard]] bool BeginSecondaryCmds(const SecondaryBufferBeginPayload& payload);

        [[nodiscard]] bool EndCmds() const;

        void ResetCmds();

        [[nodiscard]] RHIEncoder<API>* CreateCommandEncoder() { return &m_encoder; }

        //! The value is other wait or submit depending on the context
        using SubmitTimelinePayload = Shift::SubmitTimelinePayload;

        //! Also waits on every timeline point FlushPendingAcquires stored for this recording
        [[nodiscard]] bool SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads, std::span<SubmitTimelinePayload> sigSemPayloads);

        [[nodiscard]] bool SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads, std::span<SubmitTimelinePayload> sigSemPayloads, std::span<BinarySemaphore*> waitBinSems, std::span<BinarySemaphore*> sigBinSems);

        //! Call after BeginCmds, before using any handed-off texture from another queue as ut records the acquire half of every
        //! pending handoff addressed to this context's queue. This recording's submit waits their release
        void FlushPendingAcquires();

        void ExecuteSecondaryGraphicsContexts(std::span<CommandBuffer*> secondaryBuffs) const;

    private:
        EPoolQueueType GetQueueType(EContextType type);

        RHILocal<API>* m_local = nullptr;
        RHIEncoder<API> m_encoder;
        Core::UniquePtr<ResourceStateTracker<API>> m_tracker;
        //! Stored release payloads from FlushPending acquires
        std::vector<SubmitTimelinePayload> m_acquireWaits;
        Core::UniquePtr<CommandBuffer> m_cmdBuffer;
        Core::UniquePtr<CommandPool> m_cmdPool;
        EContextType m_type = EContextType::Graphics;
        bool m_isSecondary = false;
    };



    template<>
    inline bool RHIContext<RHI::Vulkan>::Init(RHILocal<RHI::Vulkan> *local, EContextType type, bool secondary) {
        m_local = local;
        m_type = type;
        m_isSecondary = secondary;
        m_cmdPool = Core::CreateUnique<CommandPool>(m_local->device.get(), GetQueueType(m_type));
        CheckCritical(m_cmdPool->IsValid(), "Failed to initialize command pool");

        m_cmdBuffer = Core::CreateUnique<CommandBuffer>(m_local->device.get(), m_local->instance.get(), *m_cmdPool, m_isSecondary);
        CheckCritical(m_cmdBuffer->IsValid(), "Failed to init command buffer!");

        m_tracker = Core::CreateUnique<ResourceStateTracker<RHI::Vulkan>>();
        m_encoder = RHIEncoder<RHI::Vulkan>{m_cmdBuffer.get(), m_tracker.get()};

        return true;
    }

    template<ValidAPI API>
    void RHIContext<API>::Destroy() {
        //! Destroy the command buffer (and with it the GPU-zone query pool it owns) here
        m_cmdBuffer.reset();
        m_cmdPool.reset();
        m_tracker.reset();
    }

    template<ValidAPI API>
    bool RHIContext<API>::BeginCmds() {
        assert(!m_isSecondary);
        m_tracker->Clear();
        m_acquireWaits.clear();
        return m_cmdBuffer->Begin();
    }

    template<ValidAPI API>
    bool RHIContext<API>::BeginSecondaryCmds(const SecondaryBufferBeginPayload &payload) {
        assert(m_isSecondary);
        m_tracker->Clear();
        m_acquireWaits.clear();
        return m_cmdBuffer->BeginSecondary(payload);
    }

    template<ValidAPI API>
    bool RHIContext<API>::EndCmds() const {
        return m_cmdBuffer->End();
    }

    template<ValidAPI API>
    void RHIContext<API>::ResetCmds() {
        m_tracker->Clear();
        m_acquireWaits.clear();
        m_cmdPool->Reset();
    }


    template<ValidAPI API>
    void RHIEncoder<API>::EndRenderPass() {
        m_boundCB->EndRenderPass();
    }

    template<ValidAPI API>
    void RHIEncoder<API>::CopyBufferToBuffer(const BufferOpDescriptor &srcBuf,
        const BufferOpDescriptor &dstBuf, uint32_t size) const
    {
        m_boundCB->CopyBufferToBuffer(srcBuf, dstBuf, size);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::FillBuffer(const BufferOpDescriptor& dstBuf, uint64_t size, uint32_t value) const {
        m_boundCB->FillBuffer(dstBuf, size, value);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::CopyBufferToTexture(const BufferOpDescriptor &srcBuf,
        const TextureCopyDescriptor &dstTex) const
    {
        m_boundCB->CopyBufferToTexture(srcBuf, dstTex);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::CopyTextureToBuffer(const TextureCopyDescriptor &srcTex,
        const BufferOpDescriptor &dstBuf) const
    {
        m_boundCB->CopyTextureToBuffer(srcTex, dstBuf);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BindVertexBuffer(const BufferOpDescriptor &buffer, uint32_t bindIdx) const {
        m_boundCB->BindVertexBuffer(buffer, bindIdx);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BindVertexBuffers(std::span<BufferOpDescriptor> buffers,
        uint32_t firstBind) const {
        m_boundCB->BindVertexBuffers(buffers, firstBind);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BindIndexBuffer(const BufferOpDescriptor &buffer, EIndexSize indexSize) const {
        m_boundCB->BindIndexBuffer(buffer, indexSize);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BindGraphicsPipeline(const Pipeline &pipeline) const {
        m_boundCB->BindGraphicsPipeline(pipeline);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BindResourceSet(const Pipeline& pipeline, uint32_t setIdx, const ResourceSet& set) const {
        m_boundCB->BindResourceSet(pipeline, setIdx, set);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::SetPushConstants(const Pipeline& pipeline, const void* data, uint32_t size, uint32_t offset) const {
        m_boundCB->SetPushConstants(pipeline, data, size, offset);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::DrawIndexed(const DrawIndexedConfig &drawConf) const {
        m_boundCB->DrawIndexed(drawConf);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::Draw(const DrawConfig &drawConf) const {
        m_boundCB->Draw(drawConf);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BindComputePipeline(const Pipeline &pipeline) const {
        m_boundCB->BindComputePipeline(pipeline);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::Dispatch(const DispatchConfig &dispatchConf) const {
        m_boundCB->Dispatch(dispatchConf);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BufferBarrier(const BufferOpDescriptor& buffer, uint64_t size, const MemoryDependency& dependency) const {
        m_boundCB->BufferBarrier(buffer, size, dependency);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::BlitTexture(const TextureBlitData &srcTexture, const TextureBlitData &dstTexture,
        const TextureBlitRegion &blitRegion, EFilterMode filter) const
    {
        m_boundCB->BlitTexture(srcTexture, dstTexture, blitRegion, filter);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::SetViewport(const Viewport& viewport) const {
        m_boundCB->SetViewport(viewport);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::SetScissor(const Rect2D& scissor) const {
        m_boundCB->SetScissor(scissor);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::PushDebugGroup(const char* label, const DebugLabelColor& color, bool timed) const {
        m_boundCB->PushDebugGroup(label, color, timed);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::PopDebugGroup() const {
        m_boundCB->PopDebugGroup();
    }

    template<ValidAPI API>
    void RHIEncoder<API>::InsertDebugLabel(const char* label, const DebugLabelColor& color) const {
        m_boundCB->InsertDebugLabel(label, color);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::PushTimeRange(const char* name) const {
        m_boundCB->PushTimeRange(name);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::PopTimeRange() const {
        m_boundCB->PopTimeRange();
    }

    template<ValidAPI API>
    bool RHIContext<API>::SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads, std::span<SubmitTimelinePayload> sigSemPayloads)
    {
        return SubmitCmds(waitSemPayloads, sigSemPayloads, {}, {});
    }

    template<ValidAPI API>
    bool RHIContext<API>::SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads,
        std::span<SubmitTimelinePayload> sigSemPayloads, std::span<BinarySemaphore*> waitBinSems,
        std::span<BinarySemaphore*> sigBinSems)
    {
        assert(!m_isSecondary);

        //! If we have repeating semaphores -> collapse them into one
        std::vector<TimelineSemaphore*> waitSems;
        std::vector<uint64_t> waitVals;
        auto addWait = [&](const SubmitTimelinePayload& p) {
            const auto existing = std::ranges::find(waitSems, p.semaphore);
            if (existing != waitSems.end()) {
                uint64_t& val = waitVals[std::distance(waitSems.begin(), existing)];
                val = std::max(val, p.value);
                return;
            }
            waitSems.push_back(p.semaphore);
            waitVals.push_back(p.value);
        };

        for (const auto& p: waitSemPayloads) { addWait(p); }
        for (const auto& p: m_acquireWaits) { addWait(p); }

        std::vector<TimelineSemaphore*> sigSems;
        std::vector<uint64_t> sigVals;
        for (auto& p: sigSemPayloads) {
            sigSems.push_back(p.semaphore);
            sigVals.push_back(p.value);
        }

        if (!m_cmdBuffer->Submit(waitSems, waitVals, sigSems, sigVals, waitBinSems, sigBinSems)) {
            return false;
        }

        //! Only succesfull submit reaches this, texture's state has been updated
        m_tracker->CommitSubmitted();

        //! We take the pending handoffs only on the succesful submit
        auto handoffs = m_cmdBuffer->TakeRecordedHandoffs();
        if (!handoffs.empty()) {
            //! A release is useless without a timeline point the acquirer can wait on.
            //! So, any timeline semaphore will do here but to keep it consistent, its the first one and we submit the first one to always be
            //! the one the Flush pending acquires function returns. This works because when we submit the next CB, we need
            //! to wait on the resources from the prev CB to be done in order to acquire their ownership
            assert(!sigSemPayloads.empty());
            const SubmitTimelinePayload& releasePoint = sigSemPayloads.front();
            m_local->QueuePendingAcquires(std::move(handoffs), releasePoint.semaphore, releasePoint.value);
        }

        return true;
    }

    template<ValidAPI API>
    void RHIContext<API>::FlushPendingAcquires() {
        assert(!m_isSecondary);
        for (const auto& pending : m_local->TakePendingAcquires(m_cmdBuffer->GetPoolType())) {
            m_cmdBuffer->AcquireQueueOwnership(pending.handoff);
            m_tracker->SetState(*pending.handoff.texture, pending.handoff.newLayout, pending.handoff.dstStage);
            //! Store acquire waits to add them to submit so the user does not have to do that shit
            m_acquireWaits.push_back(SubmitTimelinePayload{pending.releaseSemaphore, pending.releaseValue});
        }
    }

    template<ValidAPI API>
    void RHIContext<API>::ExecuteSecondaryGraphicsContexts(std::span<CommandBuffer *> secondaryBuffs) const {
        m_cmdBuffer->ExecuteSecondaryBuffers(secondaryBuffs);
    }



    template<ValidAPI API>
    void RHIEncoder<API>::BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture) {
        //! The attachment layouts are whatever this recording transitioned them to
        std::vector<EResourceLayout> colorLayouts;
        colorLayouts.reserve(colorTextures.size());
        for (const Texture* colorTexture : colorTextures) {
            colorLayouts.push_back(m_tracker->GetLayout(*colorTexture));
        }
        const EResourceLayout depthLayout = depthTexture.has_value() ? m_tracker->GetLayout(**depthTexture) : EResourceLayout::Undefined;

        m_boundCB->BeginRenderPass(desc, colorTextures, colorLayouts, depthTexture, depthLayout);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::TransitionTexture(Texture& texture, EResourceLayout newLayout,
        EPipelineStageFlags newStageFlags)
    {
        m_tracker->TransitionTexture(*m_boundCB, texture, newLayout, newStageFlags);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::ReleaseQueueOwnership(Texture& texture, EContextType dstContext,
        EResourceLayout dstLayout, EPipelineStageFlags dstStage)
    {
        const EPoolQueueType dstQueue = ToPoolQueueType(dstContext);
        //! Ignore same fanily stuff
        if (!m_boundCB->NeedsOwnershipTransfer(dstQueue)) {
            m_tracker->TransitionTexture(*m_boundCB, texture, dstLayout, dstStage);
            return;
        }

        m_boundCB->ReleaseQueueOwnership(texture, m_tracker->GetLayout(texture), m_tracker->GetStage(texture), dstQueue, dstLayout, dstStage);
        //! The rest of this recording should see the release state so that is what this call does
        m_tracker->SetState(texture, dstLayout, EPipelineStageFlags::NoneBit);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::GenerateMips(Texture& texture, EResourceLayout finalLayout,
        EPipelineStageFlags finalStage, EFilterMode filter)
    {
        m_boundCB->GenerateMips(texture, finalLayout, finalStage, filter);
        //! TODO: [FEATURE, DX12] Log just the texture for now, not all subresources
        m_tracker->SetState(texture, finalLayout, finalStage);
    }

    template<ValidAPI API>
    EPoolQueueType RHIContext<API>::GetQueueType(EContextType type) {
        return ToPoolQueueType(type);
    }

} // Shift

#endif //SHIFT_RHICONTEXT_HPP