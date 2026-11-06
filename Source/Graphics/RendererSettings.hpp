//
// Created by otrush on 9/15/2026.
//

#ifndef SHIFT_RENDERERSETTINGS_HPP
#define SHIFT_RENDERERSETTINGS_HPP

#include <glm/glm.hpp>

#include "Graphics/Shared/GPUShared.h"

namespace Shift::Graphics {
    //! Global Rendere settings to fuck around with
    struct RendererSettings {
        static constexpr float DEFAULT_EXPOSURE = -13.0f;

        GPU::ETonemapOperator tonemapOperator = GPU::ETonemapOperator::ACES;
        //! Exposure poer actually 2^EV
        float exposure = DEFAULT_EXPOSURE;
        //! Taken from Tardiff
        bool autoExposureEnabled = true;
        float autoExposureAdaptationSpeed = 1.1f;
        //! Based on Shift possible scene brightness
        float autoExposureMinEV = -16.0f;
        float autoExposureMaxEV = -4.0f;
        //! Debug Draw
        GPU::EDebugViewMode debugViewMode = GPU::EDebugViewMode::Lit;
        bool showLightIcons = true;
        float lightIconPixelSize = 20.0f;
        //! How thin are the selection brackets
        float selectionBracketFraction = 0.2f;
        //! Linear - post hdr
        glm::vec4 selectionColor{1.0f, 0.262f, 0.0f, 1.0f};
    };
}

#endif //SHIFT_RENDERERSETTINGS_HPP
