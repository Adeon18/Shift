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

        void TransitionTexture(const Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStageFlags);


        //! Kind of an interface leak, use for debug only lol
        [[nodiscard]] CommandBuffer* GetBoundCB() const {return m_boundCB;}
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
        m_cmdPool = Core::CreateUnique<CommandPool>(&m_local->device, GetQueueType(m_type));
        CheckCritical(m_cmdPool->IsValid(), "Failed to initialize command pool");

        CheckCritical(m_cmdBuffer.Init(&m_local->device, &m_local->instance, m_cmdPool, m_isSecondary),
                      "Failed to init command buffer!");
        CheckCritical(m_cmdBuffer.Init(&m_local->device, &m_local->instance, m_cmdPool, m_isSecondary),
                      "Failed to init command buffer!");

        m_encoder = RHIEncoder<RHI::Vulkan>{&m_cmdBuffer};

        return true;
    }

    template<ValidAPI API>
    void RHIContext<API>::Destroy() {
        m_cmdPool.Destroy();
    }

    template<ValidAPI API>
    bool RHIContext<API>::BeginCmds() const {
        assert(!m_isSecondary);
        return m_cmdBuffer.Begin();
    }

    template<ValidAPI API>
    bool RHIContext<API>::BeginSecondaryCmds(const SecondaryBufferBeginPayload &payload) const {
        assert(m_isSecondary);
        return m_cmdBuffer.BeginSecondary(payload);
    }

    template<ValidAPI API>
    bool RHIContext<API>::EndCmds() const {
        return m_cmdBuffer.End();
    }

    template<ValidAPI API>
    void RHIContext<API>::ResetCmds() const {
        m_cmdPool.Reset();
    }


    template<ValidAPI API>
    void RHIEncoder<API>::EndRenderPass() {
#ifdef SHIFT_VULKAN_BACKEND
        m_boundCB->VK_EndRenderPass();
#endif
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

        return m_cmdBuffer.Submit(waitSems, waitVals, sigSems, sigVals, waitBinSems, sigBinSems);
    }

    template<ValidAPI API>
    void RHIContext<API>::ExecuteSecondaryGraphicsContexts(std::span<CommandBuffer *> secondaryBuffs) const {
        m_cmdBuffer.ExecuteSecondaryBuffers(secondaryBuffs);
    }



    template<>
    inline void RHIEncoder<RHI::Vulkan>::BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture) {

        assert(desc.colorAttachments.size() == colorTextures.size());
        assert(desc.depthAttachment.has_value() == depthTexture.has_value());

        std::vector<VkRenderingAttachmentInfo> colorInfo;
        std::optional<VkRenderingAttachmentInfo> depthInfo;
        for (uint32_t i = 0; i < desc.colorAttachments.size(); i++) {
            const Texture* colTex = colorTextures[i];
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.colorAttachments[i];
            colorInfo.push_back(VK::Util::CreateRenderingAttachmentInfo(
                    colTex->GetView(),
                    VK::Util::ShiftToVKResourceLayout(colTex->GetResourceLayout()),
                    VK::Util::ShiftToVKClearColor(att.clearValue),
                    VK::Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                    VK::Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
                )
            );
        }

        if (desc.depthAttachment.has_value()) {
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.depthAttachment.value();
            *depthInfo = VK::Util::CreateRenderingAttachmentInfo(
                    (*depthTexture)->GetView(),
                    VK::Util::ShiftToVKResourceLayout((*depthTexture)->GetResourceLayout()),
                    VK::Util::ShiftToVKClearDepthStencil(att.clearValue),
                    VK::Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                    VK::Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
            );
        }

        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        if (desc.enableSecondaryCommandBuffers) { renderInfo.flags |= VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT; };
        renderInfo.renderArea = {.offset = VK::Util::ShiftToVKOffset2D(desc.offset), .extent = VK::Util::ShiftToVKExtent2D(desc.extent)};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = colorInfo.data();
        if (depthInfo.has_value()) {
            renderInfo.pDepthAttachment = &depthInfo.value();
        }

        m_boundCB->VK_BeginRenderPass(renderInfo);
    }

    template<>
    inline void RHIEncoder<RHI::Vulkan>::TransitionTexture(const Texture &texture, EResourceLayout newLayout,
        EPipelineStageFlags newStageFlags)
    {

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK::Util::ShiftToVKTextureAspect(texture.GetAspect());
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        m_boundCB->VK_TransferImageLayout(
            texture.GetImage(),
            VK::Util::ShiftToVKResourceLayout(texture.GetResourceLayout()),
            VK::Util::ShiftToVKResourceLayout(newLayout),
            texture.VK_GetStageFlags(),
            VK::Util::ShiftToVKPipelineStageFlags(newStageFlags)
        );

        texture.SetResourceLayout(newLayout);
        texture.VK_SetStageFlags(VK::Util::ShiftToVKPipelineStageFlags(newStageFlags));
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