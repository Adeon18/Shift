//
// Created by otrush on 11/4/2026.
//

#ifndef SHIFT_POSTPROCESSSYSTEM_HPP
#define SHIFT_POSTPROCESSSYSTEM_HPP

#include <cstdint>

#include "Graphics/RHI/RHI.hpp"
#include "Graphics/Managers/BufferManager.hpp"
#include "Graphics/Managers/PipelineManager.hpp"
#include "Graphics/RendererSettings.hpp"
#include "Graphics/Systems/AutoExposureSystem.hpp"
#include "Graphics/Systems/ToneMapSystem.hpp"

namespace Shift::Graphics {
    struct PostProcessInputs {
        uint64_t frameConstantsRef = 0;
        Texture& hdrColor;
        uint32_t hdrColorSlot = UINT32_MAX;
        float deltaTime = 0.0f;
        const RendererSettings& settings;
        Texture& output;
        const ResourceSet& globalSet;
    };

    //! The entire post process pipeline/transitions and order are here, each subsystem itself holds the actual pass logic
    class PostProcessSystem {
    public:
        [[nodiscard]] bool Init(PipelineManager& pipelineManager, BufferManager& bufferManager, RenderContextEncoder& initEncoder, ETextureFormat outputFormat);

        [[nodiscard]] bool Record(RenderContextEncoder& encoder, const PostProcessInputs& inputs) const;

        [[nodiscard]] const ToneMapSystem& GetToneMapSystem() const { return m_toneMap; }
        [[nodiscard]] AutoExposureSystem& GetAutoExposureSystem() { return m_autoExposure; }

    private:
        AutoExposureSystem m_autoExposure;
        ToneMapSystem m_toneMap;
    };
}

#endif //SHIFT_POSTPROCESSSYSTEM_HPP
