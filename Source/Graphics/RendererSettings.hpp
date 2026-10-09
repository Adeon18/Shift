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
        GPU::ETonemapOperator tonemapOperator = GPU::ETonemapOperator::None;
        //! Exposure poer actually 2^EV
        float exposure = 0.0f;
        //! How thin are the selection brackets
        float selectionBracketFraction = 0.2f;
        //! Linear - post hdr
        glm::vec4 selectionColor{1.0f, 0.262f, 0.0f, 1.0f};
    };
}

#endif //SHIFT_RENDERERSETTINGS_HPP
