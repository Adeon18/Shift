//
// Created by otrush on 10/17/2026.
//

#ifndef SHIFT_LIGHTICONSYSTEM_HPP
#define SHIFT_LIGHTICONSYSTEM_HPP

#include <cstdint>

#include "Graphics/RHI/RHI.hpp"
#include "Graphics/Managers/PipelineManager.hpp"
#include "Graphics/Shared/GPUShared.h"

namespace Shift::Graphics {
    struct LightIconInputs {
        uint64_t frameConstantsRef = 0;
        uint32_t lightCount = 0;
        float iconPixelSize = 20.0f;
        Texture& output;
    };

    //! One billboard icon per light
    class LightIconSystem {
    public:
        void Init(PipelineManager& pipelineManager, ETextureFormat outputFormat);

        [[nodiscard]] bool Record(RenderContextEncoder& encoder, const LightIconInputs& inputs) const;

        [[nodiscard]] PipelineHandle GetPipeline() const { return m_pipeline; }

    private:
        PipelineManager* m_pipelineManager = nullptr;
        PipelineHandle m_pipeline;
    };
}

#endif //SHIFT_LIGHTICONSYSTEM_HPP
