//
// Created by otrush on 9/18/2026.
//

#ifndef SHIFT_SHIFTSCENE_HPP
#define SHIFT_SHIFTSCENE_HPP

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Graphics/Managers/MeshHandle.hpp"

#include "NodeID.hpp"

namespace Shift {
    struct TransformProperty {
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
        //! Cached
        glm::mat4 world{1.0f};
        //! TRS
        [[nodiscard]] glm::mat4 LocalMatrix() const;
    };

    struct MeshProperty {
        Graphics::MeshHandle mesh;
        //! Scene wide material per submesh from import
        std::vector<uint32_t> materials;
    };

    //! Needed to mark transforms dirty
    struct DirtyTag {};

    class ShiftScene {
    public:
        //! entt::null parent <=> root
        NodeID CreateNode(std::string name, NodeID parent = entt::null);

        bool SetParent(NodeID node, NodeID newParent);

        [[nodiscard]] bool IsValid(NodeID node) const { return m_registry.valid(node); }

        [[nodiscard]] std::string_view GetName(NodeID node) const;
        [[nodiscard]] NodeID GetParent(NodeID node) const;
        [[nodiscard]] std::span<const NodeID> GetChildren(NodeID node) const;
        [[nodiscard]] std::span<const NodeID> GetRoots() const { return m_roots; }

        //! The node's transform or ancestors transform if node does not have one
        [[nodiscard]] glm::mat4 GetWorld(NodeID node) const;

        TransformProperty* AddTransform(NodeID node, TransformProperty transform);
        //! The place where we write transforms
        TransformProperty* EditTransform(NodeID node);

        MeshProperty* AddMesh(NodeID node, MeshProperty mesh);
        //! Same but with esh
        MeshProperty* EditMesh(NodeID node);

        //! For views to make a view, use and drop
        [[nodiscard]] const entt::registry& GetRegistry() const { return m_registry; }

        //! Recompute world transforms, parents before children
        void UpdateHierarchy();

    private:
        //! Stores what makes entity a node
        struct NodeRecord {
            std::string name;
            NodeID parent = entt::null;
            std::vector<NodeID> children;
        };

        //! TODO [Owner] Called wherever a world transform can change
        void MarkDirty(NodeID node);
        void UpdateSubtree(NodeID node, const glm::mat4& parentWorld);
        //! Check if a node or one of its ancestors is the subtree root
        [[nodiscard]] bool IsInSubtree(NodeID node, NodeID subtreeRoot) const;

        entt::registry m_registry;
        std::vector<NodeID> m_roots;
    };
}

#endif //SHIFT_SHIFTSCENE_HPP
