//
// Created by otrush on 9/18/2026.
//

#ifndef SHIFT_SHIFTSCENE_HPP
#define SHIFT_SHIFTSCENE_HPP

#include <concepts>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
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

    //! locked - imported model has hidden children
    struct ModelLockProperty {
        bool locked = true;
    };

    //! Needed to mark transforms dirty
    struct DirtyTag {};

    //! What AddProperty/EditProperty/GetProperty accept
    template<typename T>
    concept SceneProperty = std::same_as<T, TransformProperty> || std::same_as<T, MeshProperty> || std::same_as<T, ModelLockProperty>;

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

        //! nullptr if the node is invalid or already has one
        template<SceneProperty T>
        T* AddProperty(NodeID node, T property) {
            if (!IsValid(node) || m_registry.all_of<T>(node)) { return nullptr; }
            T& added = m_registry.emplace<T>(node, std::move(property));
            if constexpr (std::same_as<T, TransformProperty>) { MarkDirty(node); }
            return &added;
        }

        //! Edit property
        //! Special logic to make transforms dirty
        template<SceneProperty T>
        T* EditProperty(NodeID node) {
            if (!IsValid(node)) { return nullptr; }
            T* property = m_registry.try_get<T>(node);
            if constexpr (std::same_as<T, TransformProperty>) {
                if (property) { MarkDirty(node); }
            }
            return property;
        }

        //! Get one property or nullptr if absent
        template<SceneProperty T>
        [[nodiscard]] const T* GetProperty(NodeID node) const {
            return IsValid(node) ? m_registry.try_get<T>(node) : nullptr;
        }

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
