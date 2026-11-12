//
// Created by otrush on 11/4/2026.
//

#ifndef SHIFT_AUTOEXPOSURESYSTEM_HPP
#define SHIFT_AUTOEXPOSURESYSTEM_HPP

#include <cstdint>

#include "Graphics/RHI/RHI.hpp"
#include "Graphics/Managers/BufferManager.hpp"
#include "Graphics/Managers/PipelineManager.hpp"
#include "Graphics/Systems/CaptureSystem.hpp"

namespace Shift::Graphics {
    struct AutoExposureInputs {
        uint32_t hdrColorSlot = UINT32_MAX;
        uint32_t width = 0;
        uint32_t height = 0;
        float deltaTime = 0.0f;
        float adaptationSpeed = 1.1f;
        float minExposure = 0.0f;
        float maxExposure = 0.0f;
        const ResourceSet& globalSet;
    };

    //! Reduces the HDR target into a luminance histogram and adapts an exposure value from it.
    //! https://www.alextardif.com/HistogramLuminance.html
    class AutoExposureSystem {
    public:
        [[nodiscard]] bool Init(PipelineManager& pipelineManager, BufferManager& bufferManager,
                                RenderContextEncoder& initEncoder, float seedExposure);

        [[nodiscard]] bool Record(RenderContextEncoder& encoder, const AutoExposureInputs& inputs) const;

        [[nodiscard]] PipelineHandle GetBuildPipeline() const { return m_buildPipeline; }
        [[nodiscard]] PipelineHandle GetReducePipeline() const { return m_reducePipeline; }

        [[nodiscard]] uint64_t GetExposureRef() const;

        //! TODO [ARCHITECTURE]: THIS IS FUCKING STUPID, like man, but idont wanna write a rendergraph
        //! Every pass reading the exposure declares it here, at its own stage
        void DeclareExposureRead(RenderContextEncoder& encoder, EPipelineStageFlags stage) const;

        [[nodiscard]] BufferCaptureTarget GetHistogramCaptureTarget() const;
        [[nodiscard]] BufferCaptureTarget GetExposureCaptureTarget() const;

    private:
        void RecordBuild(RenderContextEncoder& encoder, const Pipeline& pipeline, Buffer& histogram, Buffer& exposure, const AutoExposureInputs& inputs) const;
        void RecordReduce(RenderContextEncoder& encoder, const Pipeline& pipeline, Buffer& histogram, Buffer& exposure, const AutoExposureInputs& inputs) const;

        PipelineManager* m_pipelineManager = nullptr;
        BufferManager* m_bufferManager = nullptr;

        PipelineHandle m_buildPipeline;
        PipelineHandle m_reducePipeline;
        BufferHandle m_histogram;
        BufferHandle m_exposure;
    };
}

#endif //SHIFT_AUTOEXPOSURESYSTEM_HPP
