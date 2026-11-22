#ifndef SHIFT_RHICONTEXT_DX12_HPP
#define SHIFT_RHICONTEXT_DX12_HPP

namespace Shift {
    template<>
    inline bool RHIContext<RHI::DX12>::Init(RHILocal<RHI::DX12>* local, EContextType type, bool secondary) { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline bool RHIContext<RHI::DX12>::SubmitToQueue(std::span<TimelineSemaphore*> waitSems, std::span<uint64_t> waitVals,
        std::span<TimelineSemaphore*> sigSems, std::span<uint64_t> sigVals, const FrameImage* presentImage) { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline void RHIContext<RHI::DX12>::FlushPendingAcquires() { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline void RHIEncoder<RHI::DX12>::ReleaseQueueOwnership(Texture& texture, EContextType dstContext,
        EResourceLayout dstLayout, EPipelineStageFlags dstStage) { SRHI_NOT_IMPLEMENTED(); }
} // Shift

#endif //SHIFT_RHICONTEXT_DX12_HPP
