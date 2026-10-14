//
// Created by otrush on 1/7/2026.
//

#ifndef SHIFT_VIEWPORTPANEL_HPP
#define SHIFT_VIEWPORTPANEL_HPP

#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
//! After imgui.h on purpose: ImGuizmo.h names ImVec2/ImDrawList but does not include it itself
#include "ImGuizmo.h"
#include "EditorPanel.hpp"
#include "EditorContext.hpp"
#include "Scene/ShiftScene.hpp"

namespace Shift::Editor {

    //! Reconsstrust the modelToWorld using a new world matrix provided by gizmo
    inline void SetLocalFromWorld(TransformProperty& transform, const glm::mat4& parentWorld, const glm::mat4& world) {
        const glm::mat4 local = glm::inverse(parentWorld) * world;

        transform.translation = glm::vec3(local[3]);

        glm::vec3 basis[3]{glm::vec3(local[0]), glm::vec3(local[1]), glm::vec3(local[2])};
        const glm::vec3 lengths{glm::length(basis[0]), glm::length(basis[1]), glm::length(basis[2])};
        if (glm::any(glm::lessThan(lengths, glm::vec3(1e-6f)))) { return; }

        for (uint32_t axis = 0; axis < 3; ++axis) { basis[axis] /= lengths[axis]; }
        transform.scale = lengths;
        transform.rotation = glm::quat_cast(glm::mat3(basis[0], basis[1], basis[2]));
    }

    class ViewportPanel : public EditorPanel {
    public:
        using SceneProvider = std::function<ShiftScene&()>;
        using MatrixProvider = std::function<const glm::mat4&()>;

        ViewportPanel(EditorContext& ctx, SceneProvider scene, MatrixProvider view, MatrixProvider projection)
            : EditorPanel("Viewport"), m_Context(ctx), m_sceneProvider(std::move(scene)),
              m_viewProvider(std::move(view)), m_projectionProvider(std::move(projection)) {}

        void OnImGuiRender() override {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            //! THis has to be here, otherwise rhe gizmo's setrect would map onto not an actual viewport image but just a tab lol
            if (!m_isVisible) {
                ImGui::PopStyleVar();
                return;
            }

            ImVec2 viewportSize = ImGui::GetContentRegionAvail();

            if (viewportSize.x > 0 && viewportSize.y > 0) {
                if (m_Context.EngineOutputTextureID) {
                    ImGui::Image(m_Context.EngineOutputTextureID, viewportSize);

                    const ImVec2 imageMin = ImGui::GetItemRectMin();
                    const ImVec2 imageSize = ImGui::GetItemRectSize();

                    HandleModeKeys();
                    const bool gizmoActive = DrawGizmo(imageMin, imageSize);
                    HandleSelectionClick(imageMin, imageSize, gizmoActive);
                } else {
                    ImGui::Text("No Texture ID");
                }

                uint32_t newWidth = static_cast<uint32_t>(viewportSize.x);
                uint32_t newHeight = static_cast<uint32_t>(viewportSize.y);

                if (newWidth > 0 && newHeight > 0 && (newWidth != m_LastSize.x || newHeight != m_LastSize.y)) {
                    m_Context.OnViewportResize(newWidth, newHeight);
                    m_LastSize = ImVec2{static_cast<float>(newWidth), static_cast<float>(newHeight)};
                }
            } else {
                //! We at not visible at 0x0 window
                m_isVisible = false;
            }

            //! If the Window containing this viewport is minimized we stop
            auto* rootWindow = ImGui::GetWindowViewport();
            if (rootWindow->Size.x <= 0 || rootWindow->Size.y <= 0) {
                m_isVisible = false;
            }

            ImGui::PopStyleVar();
        }

    private:
        EditorContext& m_Context;
        ImVec2 m_LastSize = {0, 0};
        SceneProvider m_sceneProvider;
        MatrixProvider m_viewProvider;
        MatrixProvider m_projectionProvider;

        void HandleModeKeys() {
            if (!ImGui::IsWindowHovered() || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::GetIO().KeyCtrl) {
                return;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_W)) { m_Context.gizmoOperation = EGizmoOperation::Translate; }
            if (ImGui::IsKeyPressed(ImGuiKey_E)) { m_Context.gizmoOperation = EGizmoOperation::Rotate; }
            if (ImGui::IsKeyPressed(ImGuiKey_R)) { m_Context.gizmoOperation = EGizmoOperation::Scale; }
            if (ImGui::IsKeyPressed(ImGuiKey_T)) { m_Context.gizmoOperation = EGizmoOperation::Universal; }
        }

        static ImGuizmo::OPERATION ToImGuizmo(EGizmoOperation operation) {
            switch (operation) {
                case EGizmoOperation::Translate: return ImGuizmo::TRANSLATE;
                case EGizmoOperation::Rotate:    return ImGuizmo::ROTATE;
                case EGizmoOperation::Scale:     return ImGuizmo::SCALE;
                case EGizmoOperation::Universal: return ImGuizmo::UNIVERSAL;
            }
            return ImGuizmo::TRANSLATE;
        }

        //! Returns whether the gizmo owns the cursor, so a click does not also reselect
        bool DrawGizmo(const ImVec2& imageMin, const ImVec2& imageSize) {
            if (!m_sceneProvider || !m_viewProvider || !m_projectionProvider) { return false; }

            ShiftScene& scene = m_sceneProvider();
            const NodeID node = m_Context.selectedNode;
            if (!scene.IsValid(node)) { return false; }

            ImGuizmo::SetDrawlist();
            ImGuizmo::SetOrthographic(false);
            //! Set the viewport and not fullscreen
            ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

            //! Otherwise clcking on a tab in front of the gizmo would stilla ctivate it
            ImGuizmo::Enable(ImGui::IsWindowHovered() || ImGuizmo::IsUsing());

            glm::mat4 world = scene.GetWorld(node);
            const bool manipulated = ImGuizmo::Manipulate(
                glm::value_ptr(m_viewProvider()),
                glm::value_ptr(m_projectionProvider()),
                ToImGuizmo(m_Context.gizmoOperation),
                ImGuizmo::WORLD,
                glm::value_ptr(world)
            );

            if (manipulated) {
                if (TransformProperty* transform = scene.EditProperty<TransformProperty>(node)) {
                    SetLocalFromWorld(*transform, scene.GetWorld(scene.GetParent(node)), world);
                }
            }

            return ImGuizmo::IsOver() || ImGuizmo::IsUsing();
        }

        void HandleSelectionClick(const ImVec2& imageMin, const ImVec2& imageSize, bool gizmoActive) {
            if (gizmoActive || !m_Context.PickNodeAt) { return; }
            if (!ImGui::IsWindowHovered() || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { return; }
            if (imageSize.x <= 0.0f || imageSize.y <= 0.0f) { return; }

            const ImVec2 mouse = ImGui::GetMousePos();
            const float u = (mouse.x - imageMin.x) / imageSize.x;
            const float v = (mouse.y - imageMin.y) / imageSize.y;
            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) { return; }

            m_Context.selectedNode = m_Context.PickNodeAt(2.0f * u - 1.0f, 1.0f - 2.0f * v);
        }
    };
}

#endif //SHIFT_VIEWPORTPANEL_HPP
