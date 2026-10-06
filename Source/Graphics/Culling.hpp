//
// Created by otrush on 10/6/2026.
//

#ifndef SHIFT_CULLING_HPP
#define SHIFT_CULLING_HPP

#include <array>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace Shift::Graphics {

    //! The six planes of a view volume
    struct Frustum {
        enum EPlane : uint32_t {
            Near = 0,
            Far,
            Left,
            Right,
            Bottom,
            Top,
            PlaneCount
        };

        //! xyz = normal (plane inside), w = distance normalized
        std::array<glm::vec4, PlaneCount> planes{};
    };

    //! Make frustum from materix
    [[nodiscard]] inline Frustum MakeFrustum(const glm::mat4& viewProj) {
        Frustum f;
        f.planes[Frustum::EPlane::Left] = glm::row(viewProj, 3) + glm::row(viewProj, 0);
        f.planes[Frustum::EPlane::Right] = glm::row(viewProj, 3) - glm::row(viewProj, 0);
        f.planes[Frustum::EPlane::Top] = glm::row(viewProj, 3) - glm::row(viewProj, 1);
        f.planes[Frustum::EPlane::Bottom] = glm::row(viewProj, 3) + glm::row(viewProj, 1);
        f.planes[Frustum::EPlane::Near] = glm::row(viewProj, 2);
        f.planes[Frustum::EPlane::Far] = glm::row(viewProj, 3) - glm::row(viewProj, 2);
        for (auto& plane: f.planes) {
            plane /= glm::length(glm::vec3(plane));
        }
        return f;
    }

    [[nodiscard]] inline bool IsVisible(const Frustum& frustum, const glm::vec4& worldSphere) {
        auto IsOutsidePlane = [](const glm::vec4& plane, const glm::vec4& sphere) {
            return glm::dot(plane, glm::vec4(glm::vec3(sphere), 1.0f)) < -sphere.w;
        };
        for (auto& plane: frustum.planes) {
            if (IsOutsidePlane(plane, worldSphere)) {
                return false;
            }
        }
        return true;
    }
}

#endif //SHIFT_CULLING_HPP
