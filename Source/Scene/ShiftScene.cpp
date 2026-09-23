//
// Created by otrush on 9/18/2026.
//

#include "ShiftScene.hpp"

#include <utility>

#include <glm/gtc/matrix_transform.hpp>

namespace Shift {
    glm::mat4 TransformProperty::LocalMatrix() const {
        return glm::translate(glm::mat4(1.0f), translation)
             * glm::mat4_cast(rotation)
             * glm::scale(glm::mat4(1.0f), scale);
    }

    NodeID ShiftScene::CreateNode(std::string name, NodeID parent) {
        const NodeID node = m_registry.create();
        const bool hasParent = IsValid(parent);
        m_registry.emplace<NodeRecord>(node, std::move(name), hasParent ? parent : NodeID{entt::null});

        if (hasParent) {
            m_registry.get<NodeRecord>(parent).children.push_back(node);
        } else {
            m_roots.push_back(node);
        }
        return node;
    }

    bool ShiftScene::SetParent(NodeID node, NodeID newParent) {
        if (!IsValid(node)) { return false; }
        const bool toRoot = !IsValid(newParent);
        //! If parent is a child of node - hehe no don't loop it then
        if (!toRoot && IsInSubtree(newParent, node)) { return false; }

        NodeRecord& record = m_registry.get<NodeRecord>(node);
        //! Avoid resetting root
        const bool isRoot = !IsValid(record.parent);
        if (toRoot ? isRoot : record.parent == newParent) { return true; }

        std::vector<NodeID>& oldSiblings = isRoot ? m_roots : m_registry.get<NodeRecord>(record.parent).children;
        std::erase(oldSiblings, node);

        if (toRoot) {
            record.parent = entt::null;
            m_roots.push_back(node);
        } else {
            record.parent = newParent;
            m_registry.get<NodeRecord>(newParent).children.push_back(node);
        }
        //! The local transform is kept, so the world one changes on the next update
        MarkDirty(node);
        return true;
    }

    std::string_view ShiftScene::GetName(NodeID node) const {
        return IsValid(node) ? std::string_view{m_registry.get<NodeRecord>(node).name} : std::string_view{};
    }

    NodeID ShiftScene::GetParent(NodeID node) const {
        return IsValid(node) ? m_registry.get<NodeRecord>(node).parent : NodeID{entt::null};
    }

    std::span<const NodeID> ShiftScene::GetChildren(NodeID node) const {
        if (!IsValid(node)) { return {}; }
        return m_registry.get<NodeRecord>(node).children;
    }

    glm::mat4 ShiftScene::GetWorld(NodeID node) const {
        for (NodeID n = node; IsValid(n); n = m_registry.get<NodeRecord>(n).parent) {
            if (const TransformProperty* transform = m_registry.try_get<TransformProperty>(n)) {
                return transform->world;
            }
        }
        return glm::mat4(1.0f);
    }

    void ShiftScene::MarkDirty(NodeID node) {
        m_registry.emplace_or_replace<DirtyTag>(node);
    }

    //! Get only the dirty components, update them with respect to parent
    void ShiftScene::UpdateHierarchy() {
        m_registry.view<DirtyTag>().each([this](const auto entity) {
            UpdateSubtree(entity, GetWorld(GetParent(entity)));
        });
        m_registry.clear<DirtyTag>();
    }

    //! TODO: [OPTIMIZATION] This can be NOT recursive
    void ShiftScene::UpdateSubtree(NodeID node, const glm::mat4& parentWorld) {
        glm::mat4 world = parentWorld;
        if (TransformProperty* transform = m_registry.try_get<TransformProperty>(node)) {
            transform->world = parentWorld * transform->LocalMatrix();
            world = transform->world;
        }
        for (const NodeID child : m_registry.get<NodeRecord>(node).children) {
            UpdateSubtree(child, world);
        }
    }

    bool ShiftScene::IsInSubtree(NodeID node, NodeID subtreeRoot) const {
        for (NodeID n = node; IsValid(n); n = m_registry.get<NodeRecord>(n).parent) {
            if (n == subtreeRoot) { return true; }
        }
        return false;
    }
}
