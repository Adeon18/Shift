//
// Created by otrush on 9/14/2026.
//

#ifndef SHIFT_TONEMAPSYSTEM_HPP
#define SHIFT_TONEMAPSYSTEM_HPP

#include <cstdint>

#include "Graphics/RHI/RHI.hpp"
#include "Graphics/Managers/PipelineManager.hpp"

namespace Shift::Graphics {
    struct ToneMapInputs {
        uint64_t frameConstantsRef = 0;
        uint32_t hdrColorSlot = UINT32_MAX;
        Texture& output;
        const ResourceSet& globalSet;
    };

    //! Turns HDR scene color to the ldr color
    class ToneMapSystem {
    public:
        void Init(PipelineManager& pipelineManager, ETextureFormat outputFormat);

        [[nodiscard]] bool Record(RenderContextEncoder& encoder, const ToneMapInputs& inputs) const;

        [[nodiscard]] PipelineHandle GetPipeline() const { return m_pipeline; }

    private:
        PipelineManager* m_pipelineManager = nullptr;
        PipelineHandle m_pipeline;
    };
}

#endif //SHIFT_TONEMAPSYSTEM_HPP
