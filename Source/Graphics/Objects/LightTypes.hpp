//
// Created by otrush on 3/18/2024.
//

#ifndef SHIFT_LIGHTTYPES_HPP
#define SHIFT_LIGHTTYPES_HPP

#include <glm/glm.hpp>

namespace Shift::gfx {
    struct DirectionalLight {
        glm::vec4 direction;
        glm::vec4 radiance;
    };

    struct PointLight {
        glm::vec4 position;
        glm::vec4 radiance;
    };
}

#endif //SHIFT_LIGHTTYPES_HPP
