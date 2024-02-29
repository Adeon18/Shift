#pragma once

#include <glm/glm.hpp>

namespace shift::gfx {
    struct PerFrame {
        /// These are vec3, for alignment
        glm::vec4 camPosition;
        glm::vec4 camDirection;
        glm::vec4 camRight;
        glm::vec4 camUp;
        /// Window data, xy - width/height; zw - 1/width\height
        glm::vec4 windowData;
        /// Timer data, x - dt, y - fps, z - seconds dince start
        glm::vec4 timerData;
    };

    struct PerDefaultView {
        glm::mat4 view;
        glm::mat4 proj;
        glm::mat4 viewInv;
        glm::mat4 projInv;
    };

    //! TODO: will change with instancing
    struct PerDefaultObject {
        glm::mat4 meshToModel;
        glm::mat4 meshToModelInv;
        glm::mat4 modelToWorld;
        glm::mat4 modelToWorldInv;
    };
} // shift::gfx