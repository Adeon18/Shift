//
// Created by otrush on 10/14/2026.
//

#ifndef SHIFT_SCENEPICKER_HPP
#define SHIFT_SCENEPICKER_HPP

#include <optional>

#include <glm/glm.hpp>

#include "Scene/NodeID.hpp"

namespace Shift {
    class ShiftScene;
}

namespace Shift::Graphics {
    class MeshManager;

    struct WorldAABB {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
    };

    struct Ray {
        glm::vec3 origin{0.0f};
        glm::vec3 direction{0.0f};
    };

    //! AABB of subtree
    [[nodiscard]] std::optional<WorldAABB> SubtreeWorldAABB(const ShiftScene& scene, const MeshManager& meshes, NodeID node);

    //! Make x right y up ray
    [[nodiscard]] Ray MakeViewportRay(const glm::mat4& viewProj, float ndcX, float ndcY);

    //! Select either the lowest locked ansestor or the node. is here so that we can pick non-root nodes and avoid editing locked nodes
    [[nodiscard]] NodeID SelectableAncestor(const ShiftScene& scene, NodeID node);

    //! Nearest submesh box that ray enters, NOT pixel perfect
    [[nodiscard]] NodeID PickNode(const ShiftScene& scene, const MeshManager& meshes, const Ray& ray);
}

#endif //SHIFT_SCENEPICKER_HPP
