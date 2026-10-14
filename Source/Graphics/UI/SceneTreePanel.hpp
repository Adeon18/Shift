//
// Created by otrush on 9/22/2026.
//

#ifndef SHIFT_SCENETREEPANEL_HPP
#define SHIFT_SCENETREEPANEL_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

#include "imgui/imgui.h"
#include "EditorPanel.hpp"
#include "EditorContext.hpp"

#include "Scene/ShiftScene.hpp"

//! The node hierarchy + select logic
namespace Shift::Editor {
    class SceneTreePanel : public EditorPanel {
    public:
        using SceneProvider = std::function<const ShiftScene&()>;

        SceneTreePanel(EditorContext& ctx, SceneProvider provider)
            : EditorPanel("Scene Hierarchy"), m_context(ctx), m_provider(std::move(provider)) {}

        void OnImGuiRender() override {
            //! IsVisible check here is not to miss the scene tree open logic atmouse selection
            if (!m_provider || !m_isVisible) { return; }
            const ShiftScene& scene = m_provider();

            UpdateReveal(scene);

            for (const NodeID root : scene.GetRoots()) {
                DrawNode(scene, root);
            }
        }

    private:
        //! TODO [CONSPECT]
        //! Force open the scene tree UI when selecting an object by mouse by storing the entire chain of nodes that
        //! need to be revealed to reach the selected node and then at draw checking if we are iterating that chain and revealing the nodes
        void UpdateReveal(const ShiftScene& scene) {
            const NodeID selected = m_context.selectedNode;
            m_revealTarget = NULL_NODE;
            m_revealChain.clear();

            if (selected == m_lastSelected) { return; }
            m_lastSelected = selected;
            if (!scene.IsValid(selected)) { return; }

            m_revealTarget = selected;
            for (NodeID n = scene.GetParent(selected); n != NULL_NODE; n = scene.GetParent(n)) {
                m_revealChain.push_back(n);
            }
        }

        void DrawNode(const ShiftScene& scene, NodeID node) {
            const std::span<const NodeID> children = scene.GetChildren(node);

            const ModelLockProperty* lockProp = scene.GetProperty<ModelLockProperty>(node);
            const bool hideChildren = (lockProp && lockProp->locked);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (node == m_context.selectedNode) { flags |= ImGuiTreeNodeFlags_Selected; }
            if (children.empty() || hideChildren) { flags |= ImGuiTreeNodeFlags_Leaf; }

            std::string_view name = scene.GetName(node);
            if (name.empty()) { name = "(unnamed)"; }

            //! Iterate the reveal chain we logged earlier
            const bool onRevealPath = std::find(m_revealChain.begin(), m_revealChain.end(), node) != m_revealChain.end();
            if (onRevealPath) { ImGui::SetNextItemOpen(true); }

            //! Has to be unique id
            const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(node)), flags,"%.*s", static_cast<int>(name.size()), name.data());

            //! selection
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_context.selectedNode = node;
                m_lastSelected = node;
            }

            if (node == m_revealTarget) { ImGui::SetScrollHereY(0.5f); }

            if (!open) { return; }
            if (!hideChildren) {
                for (const NodeID child : children) {
                    DrawNode(scene, child);
                }
            }
            ImGui::TreePop();
        }

        EditorContext& m_context;
        SceneProvider m_provider;
        NodeID m_lastSelected = NULL_NODE;
        NodeID m_revealTarget = NULL_NODE;
        std::vector<NodeID> m_revealChain;
    };
}

#endif //SHIFT_SCENETREEPANEL_HPP
