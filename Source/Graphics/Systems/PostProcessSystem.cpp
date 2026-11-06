//
// Created by otrush on 11/4/2026.
//

#include "PostProcessSystem.hpp"

#include <cmath>

#include "Utility/Assertions.hpp"

namespace Shift::Graphics {

    bool PostProcessSystem::Init(PipelineManager& pipelineManager, BufferManager& bufferManager, RenderContextEncoder& initEncoder, ETextureFormat outputFormat) {
        CheckCritical(m_autoExposure.Init(pipelineManager, bufferManager, initEncoder, std::exp2(RendererSettings::DEFAULT_EXPOSURE)), "Failed to initialize the auto-exposure stage!");
        m_toneMap.Init(pipelineManager, outputFormat);
        return true;
    }

    bool PostProcessSystem::Record(RenderContextEncoder& encoder, const PostProcessInputs& inputs) const {
        const RendererSettings& settings = inputs.settings;
        const bool isLit = settings.debugViewMode == GPU::EDebugViewMode::Lit;

        //! WARNING: DO NOT FORGET TO EDIT THIS IF NEW PASSES NEED DIFF STAGE FLAGS
        encoder.TransitionTexture(inputs.hdrColor, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::ComputeShaderBit | EPipelineStageFlags::FragmentShaderBit);

        if (isLit && settings.autoExposureEnabled) {
            const AutoExposureInputs meterInputs{
                .hdrColorSlot = inputs.hdrColorSlot,
                .width = inputs.hdrColor.GetWidth(),
                .height = inputs.hdrColor.GetHeight(),
                .deltaTime = inputs.deltaTime,
                .adaptationSpeed = settings.autoExposureAdaptationSpeed,
                .minExposure = settings.autoExposureMinEV,
                .maxExposure = settings.autoExposureMaxEV,
                .globalSet = inputs.globalSet
            };
            CheckCritical(m_autoExposure.Record(encoder, meterInputs), "Failed to record the auto-exposure stage!");
        }

        const ToneMapInputs toneMapInputs{
            .frameConstantsRef = inputs.frameConstantsRef,
            .hdrColorSlot = inputs.hdrColorSlot,
            .tonemapOperator = isLit ? settings.tonemapOperator : GPU::ETonemapOperator::None,
            .output = inputs.output,
            .globalSet = inputs.globalSet
        };
        CheckCritical(m_toneMap.Record(encoder, toneMapInputs), "Failed to record the tone map stage!");

        return true;
    }
}
