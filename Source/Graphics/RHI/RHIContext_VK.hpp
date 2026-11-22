#ifndef SHIFT_RHICONTEXT_VK_HPP
#define SHIFT_RHICONTEXT_VK_HPP

//! Vulkan specializations of RHIContext<RHI::Vulkan> / RHIEncoder<RHI::Vulkan> members, included at the end of RHIContext.hpp

namespace Shift {

    template<>
    inline bool RHIContext<RHI::Vulkan>::Init(RHILocal<RHI::Vulkan> *local, EContextType type, bool secondary) {
        m_local = local;
        m_type = type;
        m_isSecondary = secondary;
        m_cmdPool = RHI::CreateUnique<CommandPool>(m_local->device.get(), GetQueueType(m_type));
        SRHI_CHECK_CRITICAL(m_cmdPool->IsValid(), "Failed to initialize command pool");

        m_cmdBuffer = RHI::CreateUnique<CommandBuffer>(m_local->device.get(), m_local->instance.get(), *m_cmdPool, m_isSecondary);
        SRHI_CHECK_CRITICAL(m_cmdBuffer->IsValid(), "Failed to init command buffer!");

        m_tracker = RHI::CreateUnique<ResourceStateTracker<RHI::Vulkan>>();
        m_encoder = RHIEncoder<RHI::Vulkan>{m_cmdBuffer.get(), m_tracker.get()};

        return true;
    }

    template<>
    inline bool RHIContext<RHI::Vulkan>::SubmitToQueue(std::span<TimelineSemaphore*> waitSems, std::span<uint64_t> waitVals,
        std::span<TimelineSemaphore*> sigSems, std::span<uint64_t> sigVals, const FrameImage* presentImage)
    {
        bool submitted = false;
        if (presentImage == nullptr) {
            submitted = m_cmdBuffer->Submit(waitSems, waitVals, sigSems, sigVals);
        } else {
            std::array<VK::BinarySemaphore*, 1> imageAvailable{m_local->lastAcquireSemaphore};
            std::array<VK::BinarySemaphore*, 1> renderFinished{m_local->renderFinished[presentImage->index].get()};
            assert(imageAvailable[0] != nullptr);
            submitted = m_cmdBuffer->Submit(waitSems, waitVals, sigSems, sigVals, imageAvailable, renderFinished);
        }
        if (!submitted) {
            return false;
        }

        //! The acquire halves exist for other queues only once their release is submitted. Any signalled
        //! timeline is a point the acquirer can wait on, the first one it is
        std::vector<VK::QueueOwnershipHandoffPayload> handoffs = m_cmdBuffer->TakeRecordedHandoffs();
        if (!handoffs.empty()) {
            assert(!sigSems.empty());
            m_local->QueuePendingAcquires(std::move(handoffs), sigSems.front(), sigVals.front());
        }
        return true;
    }

    template<>
    inline void RHIContext<RHI::Vulkan>::FlushPendingAcquires() {
        assert(!m_isSecondary);
        for (const auto& pending : m_local->TakePendingAcquires(m_cmdBuffer->GetPoolType())) {
            m_cmdBuffer->AcquireQueueOwnership(pending.handoff);
            m_tracker->SetState(*pending.handoff.texture, pending.handoff.newLayout, pending.handoff.dstStage);
            //! Store acquire waits to add them to submit so the user does not have to do that shit
            m_acquireWaits.push_back(SubmitTimelinePayload{pending.releaseSemaphore, pending.releaseValue});
        }
    }

    template<>
    inline void RHIEncoder<RHI::Vulkan>::ReleaseQueueOwnership(Texture& texture, EContextType dstContext,
        EResourceLayout dstLayout, EPipelineStageFlags dstStage)
    {
        const EPoolQueueType dstQueue = ToPoolQueueType(dstContext);
        //! A shared family needs no ownership transfer, a plain transition does
        if (!m_boundCB->NeedsOwnershipTransfer(dstQueue)) {
            m_tracker->TransitionTexture(*m_boundCB, texture, dstLayout, dstStage);
            return;
        }

        m_boundCB->ReleaseQueueOwnership(texture, m_tracker->GetLayout(texture), m_tracker->GetStage(texture), dstQueue, dstLayout, dstStage);
        //! The rest of this recording sees the released state, dstStage belongs to the other queue
        m_tracker->SetState(texture, dstLayout, EPipelineStageFlags::NoneBit);
    }
} // Shift

#endif //SHIFT_RHICONTEXT_VK_HPP
