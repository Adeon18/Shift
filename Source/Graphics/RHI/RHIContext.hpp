//
// Created by otrush on 11/10/2025.
//

#ifndef SHIFT_RHICONTEXT_HPP
#define SHIFT_RHICONTEXT_HPP

#include <concepts>
#include <array>

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

    //! Forward dec for further per API local constructs (device, instance, some descriptor specific stuff)
    template<ValidAPI API> struct RHILocal {};

} // Shift

#ifdef SHIFT_VULKAN_BACKEND
#include "Vulkan/VKRHI_Impl.hpp"
#include "RHILocal_VK.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"
#endif


namespace Shift {

    enum class EContextType { Graphics, Compute, Transfer };

    template<ValidAPI API>
    class RHIEncoder {
    public:
        RHIEncoder() = default;
        explicit RHIEncoder(CommandBuffer* cmd): m_boundCB{cmd} {}
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

        //! Copy buffer data to a texture
        //! \param srcBuf buffer + offset into the buffer
        //! \param dstTex texture + size to copy + offset + subresource range
        void CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const;

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
    private:
        CommandBuffer* m_boundCB = nullptr;
    };

    template<ValidAPI API>
    class RHIContext {
    public:
        RHIContext() = default;

        // Init for primary or secondary context
        bool Init(RHILocal<API>* local, EContextType type, bool secondary = false);
        void Destroy();

        [[nodiscard]] CommandBuffer& GetCommandBuffer() {return *m_cmdBuffer;}

        [[nodiscard]] bool BeginCmds() const;

        [[nodiscard]] bool BeginSecondaryCmds(const SecondaryBufferBeginPayload& payload) const;

        [[nodiscard]] bool EndCmds() const;

        void ResetCmds() const;

        [[nodiscard]] RHIEncoder<API>* CreateCommandEncoder() { return &m_encoder; }

        //! The value is other wait or submit depending on the context
        struct SubmitTimelinePayload {
            TimelineSemaphore* semaphore = nullptr;
            uint64_t value = 0;
        };

        [[nodiscard]] bool SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads, std::span<SubmitTimelinePayload> sigSemPayloads) const;

        [[nodiscard]] bool SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads, std::span<SubmitTimelinePayload> sigSemPayloads, std::span<BinarySemaphore*> waitBinSems, std::span<BinarySemaphore*> sigBinSems) const;

        void ExecuteSecondaryGraphicsContexts(std::span<CommandBuffer*> secondaryBuffs) const;

    private:

        EPoolQueueType GetQueueType(EContextType type);


        RHILocal<API>* m_local = nullptr;
        RHIEncoder<API> m_encoder;
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

        m_encoder = RHIEncoder<RHI::Vulkan>{m_cmdBuffer.get()};

        return true;
    }

    template<ValidAPI API>
    void RHIContext<API>::Destroy() {
        //! Destroy the command buffer (and with it the GPU-zone query pool it owns) here
        m_cmdBuffer.reset();
        m_cmdPool.reset();
    }

    template<ValidAPI API>
    bool RHIContext<API>::BeginCmds() const {
        assert(!m_isSecondary);
        return m_cmdBuffer->Begin();
    }

    template<ValidAPI API>
    bool RHIContext<API>::BeginSecondaryCmds(const SecondaryBufferBeginPayload &payload) const {
        assert(m_isSecondary);
        return m_cmdBuffer->BeginSecondary(payload);
    }

    template<ValidAPI API>
    bool RHIContext<API>::EndCmds() const {
        return m_cmdBuffer->End();
    }

    template<ValidAPI API>
    void RHIContext<API>::ResetCmds() const {
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
    void RHIEncoder<API>::CopyBufferToTexture(const BufferOpDescriptor &srcBuf,
        const TextureCopyDescriptor &dstTex) const
    {
        m_boundCB->CopyBufferToTexture(srcBuf, dstTex);
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
    void RHIEncoder<API>::DrawIndexed(const DrawIndexedConfig &drawConf) const {
        m_boundCB->DrawIndexed(drawConf);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::Draw(const DrawConfig &drawConf) const {
        m_boundCB->Draw(drawConf);
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
    bool RHIContext<API>::SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads,
        std::span<SubmitTimelinePayload> sigSemPayloads) const
    {
        return SubmitCmds(waitSemPayloads, sigSemPayloads, {}, {});
    }

    template<ValidAPI API>
    bool RHIContext<API>::SubmitCmds(std::span<SubmitTimelinePayload> waitSemPayloads,
        std::span<SubmitTimelinePayload> sigSemPayloads, std::span<BinarySemaphore*> waitBinSems,
        std::span<BinarySemaphore*> sigBinSems) const
    {
        assert(!m_isSecondary);

        std::vector<TimelineSemaphore*> waitSems;
        std::vector<uint64_t> waitVals;

        for (auto& p: waitSemPayloads) {
            waitSems.push_back(p.semaphore);
            waitVals.push_back(p.value);
        }

        std::vector<TimelineSemaphore*> sigSems;
        std::vector<uint64_t> sigVals;
        for (auto& p: sigSemPayloads) {
            sigSems.push_back(p.semaphore);
            sigVals.push_back(p.value);
        }

        return m_cmdBuffer->Submit(waitSems, waitVals, sigSems, sigVals, waitBinSems, sigBinSems);
    }

    template<ValidAPI API>
    void RHIContext<API>::ExecuteSecondaryGraphicsContexts(std::span<CommandBuffer *> secondaryBuffs) const {
        m_cmdBuffer->ExecuteSecondaryBuffers(secondaryBuffs);
    }



    template<ValidAPI API>
    void RHIEncoder<API>::BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture) {
        m_boundCB->BeginRenderPass(desc, colorTextures, depthTexture);
    }

    template<ValidAPI API>
    void RHIEncoder<API>::TransitionTexture(Texture& texture, EResourceLayout newLayout,
        EPipelineStageFlags newStageFlags)
    {
        m_boundCB->TransitionTexture(texture, newLayout, newStageFlags);
    }

    template<ValidAPI API>
    EPoolQueueType RHIContext<API>::GetQueueType(EContextType type) {
        switch (type) {
            case EContextType::Graphics: return EPoolQueueType::Graphics;
            case EContextType::Compute:  return EPoolQueueType::Compute;
            case EContextType::Transfer: return EPoolQueueType::Transfer;
        }
        return EPoolQueueType::Graphics;
    }

} // Shift

#endif //SHIFT_RHICONTEXT_HPP