//
// Created by otrush on 9/15/2026.
//

#ifndef SHIFT_RENDERERSETTINGS_HPP
#define SHIFT_RENDERERSETTINGS_HPP

#include "Graphics/Shared/GPUShared.h"

namespace Shift::Graphics {
    //! Global Rendere settings to fuck around with
    struct RendererSettings {
        GPU::ETonemapOperator tonemapOperator = GPU::ETonemapOperator::None;
        //! Exposure poer actually 2^EV
        float exposure = 0.0f;
    };
}

#endif //SHIFT_RENDERERSETTINGS_HPP
