#ifndef SHIFT_RHI_DX12_HPP
#define SHIFT_RHI_DX12_HPP

namespace Shift {
    template<>
    inline void RenderHardwareInterface<RHI::DX12>::WaitForGPU() { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline const RHICapabilities& RenderHardwareInterface<RHI::DX12>::GetCapabilities() const { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline Pipeline* RenderHardwareInterface<RHI::DX12>::HandleCreator::CreatePipeline(const PipelineDescriptor& desc,
        const std::vector<ShaderStageDesc>& shaders) { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline ResourceSet* RenderHardwareInterface<RHI::DX12>::HandleCreator::CreateResourceSet(const PipelineLayoutDescriptor& desc) { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline EAcquireResult RenderHardwareInterface<RHI::DX12>::AcquireFrameImage(FrameImage& out) { SRHI_NOT_IMPLEMENTED(); }

    template<>
    inline bool RenderHardwareInterface<RHI::DX12>::Present(const FrameImage& image, bool* needsResize) { SRHI_NOT_IMPLEMENTED(); }
} // Shift

#endif //SHIFT_RHI_DX12_HPP
