//
// Created by otrush on 9/22/2026.
//

#ifndef SHIFT_NODEINSPECTORPANEL_HPP
#define SHIFT_NODEINSPECTORPANEL_HPP

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <functional>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "imgui/imgui.h"
#include "EditorPanel.hpp"
#include "EditorContext.hpp"

#include "Graphics/Managers/MaterialManager.hpp"
#include "Scene/ShiftScene.hpp"

//! The selected node's properties. Uses GetProperty, and EditProperty on edit
namespace Shift::Editor {
    class NodeInspectorPanel : public EditorPanel {
    public:
        using SceneProvider = std::function<ShiftScene&()>;
        using MaterialsProvider = std::function<const Graphics::MaterialManager&()>;

        NodeInspectorPanel(EditorContext& ctx, SceneProvider scene, MaterialsProvider materials)
            : EditorPanel("Properties"), m_context(ctx), m_sceneProvider(std::move(scene)), m_materialsProvider(std::move(materials)) {}

        void OnImGuiRender() override {
            if (!m_sceneProvider || !m_materialsProvider) { return; }
            ShiftScene& scene = m_sceneProvider();

            const NodeID node = m_context.selectedNode;
            if (!scene.IsValid(node)) {
                ImGui::TextDisabled("Nothing selected");
                return;
            }

            const std::string_view name = scene.GetName(node);
            ImGui::TextUnformatted(name.data(), name.data() + name.size());

            DrawModelLock(scene, node);
            DrawTransform(scene, node);
            DrawMesh(scene, node, m_materialsProvider());
            DrawLight(scene, node);
        }

    private:
        void DrawModelLock(ShiftScene& scene, NodeID node) {
            const ModelLockProperty* lock = scene.GetProperty<ModelLockProperty>(node);
            if (!lock) { return; }

            ImGui::SeparatorText("Model");
            bool locked = lock->locked;
            if (ImGui::Checkbox("Locked", &locked)) {
                if (ModelLockProperty* edit = scene.EditProperty<ModelLockProperty>(node)) { edit->locked = locked; }
            }
        }

        void DrawTransform(ShiftScene& scene, NodeID node) {
            ImGui::SeparatorText("Transform");
            const TransformProperty* transform = scene.GetProperty<TransformProperty>(node);
            if (!transform) {
                ImGui::TextDisabled("None, follows its parent");
                return;
            }

            glm::vec3 translation = transform->translation;
            if (ImGui::DragFloat3("Translation", &translation.x, 0.01f)) {
                if (TransformProperty* edit = scene.EditProperty<TransformProperty>(node)) { edit->translation = translation; }
            }

            SyncEuler(node, transform->rotation);
            if (ImGui::DragFloat3("Rotation", &m_eulerDegrees.x, 0.5f)) {
                const glm::quat rotation(glm::radians(m_eulerDegrees));
                if (TransformProperty* edit = scene.EditProperty<TransformProperty>(node)) { edit->rotation = rotation; }
                m_eulerRotation = rotation;
            }

            glm::vec3 scale = transform->scale;
            if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
                if (TransformProperty* edit = scene.EditProperty<TransformProperty>(node)) { edit->scale = scale; }
            }
        }

        void DrawMesh(ShiftScene& scene, NodeID node, const Graphics::MaterialManager& materials) {
            const MeshProperty* mesh = scene.GetProperty<MeshProperty>(node);
            if (!mesh) { return; }

            ImGui::SeparatorText("Mesh");
            //! Extract indexes materials by submesh
            for (size_t i = 0; i < mesh->materials.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                const uint32_t current = mesh->materials[i];
                const std::string label = "Submesh " + std::to_string(i);
                if (ImGui::BeginCombo(label.c_str(), MaterialLabel(materials, current).c_str())) {
                    for (uint32_t m = 0; m < materials.GetCount(); ++m) {
                        const bool isCurrent = m == current;
                        if (ImGui::Selectable(MaterialLabel(materials, m).c_str(), isCurrent)) {
                            if (MeshProperty* edit = scene.EditProperty<MeshProperty>(node)) { edit->materials[i] = m; }
                        }
                        if (isCurrent) { ImGui::SetItemDefaultFocus(); }
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }
        }

        void DrawLight(ShiftScene& scene, NodeID node) {
            const LightProperty* light = scene.GetProperty<LightProperty>(node);
            if (!light) { return; }

            ImGui::SeparatorText("Light");

            static const char* TYPE_NAMES[] = {"Directional", "Point", "Spot"};
            int type = static_cast<int>(light->type);
            if (ImGui::Combo("Type", &type, TYPE_NAMES, IM_ARRAYSIZE(TYPE_NAMES))) {
                if (LightProperty* edit = scene.EditProperty<LightProperty>(node)) { edit->type = static_cast<GPU::ELightType>(type); }
            }

            const bool isDirectional = light->type == GPU::ELightType::Directional;
            const bool isSpot = light->type == GPU::ELightType::Spot;

            glm::vec3 color = light->color;
            if (ImGui::ColorEdit3("Color", &color.x)) {
                if (LightProperty* edit = scene.EditProperty<LightProperty>(node)) { edit->color = color; }
            }

            const bool useLumens = m_useLumens && !isDirectional;
            const float powerFactor = LumensPerCandela(light->type);
            float intensity = useLumens ? light->intensity * powerFactor : light->intensity;
            const char* intensityLabel = isDirectional ? "Intensity (lx)" : (useLumens ? "Intensity (lm)" : "Intensity (cd)");
            const float intensitySpeed = std::max(0.01f, std::abs(intensity) * 0.01f);
            if (ImGui::DragFloat(intensityLabel, &intensity, intensitySpeed, 0.0f, FLT_MAX, "%.2f")) {
                if (LightProperty* edit = scene.EditProperty<LightProperty>(node)) {
                    edit->intensity = useLumens ? intensity / powerFactor : intensity;
                }
            }

            ImGui::BeginDisabled(isDirectional);
            ImGui::Checkbox("use Lumens", &m_useLumens);
            ImGui::EndDisabled();
            if (isDirectional) {
                ImGui::SameLine();
                ImGui::TextDisabled("(point/spot only)");
            }

            //! Range
            if (!isDirectional) {
                float range = light->range;
                if (ImGui::DragFloat("Range", &range, 0.1f, 0.01f, FLT_MAX, "%.2f")) {
                    if (LightProperty* edit = scene.EditProperty<LightProperty>(node)) { edit->range = range; }
                }
            }

            //! Spot deg
            if (isSpot) {
                float innerDegrees = glm::degrees(light->innerAngle);
                if (ImGui::DragFloat("Inner angle", &innerDegrees, 0.5f, 0.0f, 90.0f, "%.1f deg")) {
                    if (LightProperty* edit = scene.EditProperty<LightProperty>(node)) { edit->innerAngle = glm::radians(innerDegrees); }
                }
                float outerDegrees = glm::degrees(light->outerAngle);
                if (ImGui::DragFloat("Outer angle", &outerDegrees, 0.5f, 0.0f, 90.0f, "%.1f deg")) {
                    if (LightProperty* edit = scene.EditProperty<LightProperty>(node)) { edit->outerAngle = glm::radians(outerDegrees); }
                }
            }
        }

        //! TODO [CONSPECT]: Candela is lumens per steradiam, where steredian is the 3D solig angle
        //! lm = cd * sr => sr = lm/cd. 4Pi is the whole sphere
        static float LumensPerCandela(GPU::ELightType type) {
            constexpr float PI = glm::pi<float>();
            return type == GPU::ELightType::Spot ? PI : 4.0f * PI;
        }

        //! Keep labels uniquee - needed for ID
        static std::string MaterialLabel(const Graphics::MaterialManager& materials, uint32_t index) {
            const std::string_view name = materials.GetName(index);
            return "#" + std::to_string(index) + " " + (name.empty() ? std::string("(unnamed)") : std::string(name));
        }

        //! If rotation changed - manually re-derive angles because I am fucking stupididddd and decided to use quats
        void SyncEuler(NodeID node, const glm::quat& rotation) {
            if (node == m_eulerNode && rotation == m_eulerRotation) { return; }
            m_eulerNode = node;
            m_eulerRotation = rotation;
            m_eulerDegrees = glm::degrees(glm::eulerAngles(rotation));
        }

        EditorContext& m_context;
        SceneProvider m_sceneProvider;
        MaterialsProvider m_materialsProvider;

        //! Whether we want to use lumens for this light
        bool m_useLumens = false;

        //! For syncing quat rot and regular rot
        NodeID m_eulerNode = NULL_NODE;
        glm::quat m_eulerRotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 m_eulerDegrees{0.0f};
    };
}

#endif //SHIFT_NODEINSPECTORPANEL_HPP
