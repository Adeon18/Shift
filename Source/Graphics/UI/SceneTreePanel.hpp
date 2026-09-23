//
// Created by otrush on 9/22/2026.
//

#ifndef SHIFT_SCENETREEPANEL_HPP
#define SHIFT_SCENETREEPANEL_HPP

#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

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
            if (!m_provider) { return; }
            const ShiftScene& scene = m_provider();

            for (const NodeID root : scene.GetRoots()) {
                DrawNode(scene, root);
            }
        }

    private:
        void DrawNode(const ShiftScene& scene, NodeID node) {
            const std::span<const NodeID> children = scene.GetChildren(node);

            const ModelLockProperty* lockProp = scene.GetProperty<ModelLockProperty>(node);
            const bool hideChildren = (lockProp && lockProp->locked);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            if (node == m_context.selectedNode) { flags |= ImGuiTreeNodeFlags_Selected; }
            if (children.empty() || hideChildren) { flags |= ImGuiTreeNodeFlags_Leaf; }

            std::string_view name = scene.GetName(node);
            if (name.empty()) { name = "(unnamed)"; }

            //! Has to be unique id
            const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(node)), flags,"%.*s", static_cast<int>(name.size()), name.data());

            //! selection
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_context.selectedNode = node;
            }

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
    };
}

#endif //SHIFT_SCENETREEPANEL_HPP
