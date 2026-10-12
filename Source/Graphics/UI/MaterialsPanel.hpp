//
// Created by otrush on 10/12/2026.
//

#ifndef SHIFT_MATERIALSPANEL_HPP
#define SHIFT_MATERIALSPANEL_HPP

#include <algorithm>
#include <functional>
#include <string_view>
#include <vector>

#include "imgui/imgui.h"
#include "EditorPanel.hpp"
#include "EditorContext.hpp"

#include "Graphics/Managers/MaterialManager.hpp"
#include "Scene/ShiftScene.hpp"

namespace Shift::Editor {
    //! Gather material indices under node
    inline void AppendSubtreeMaterials(const ShiftScene& scene, NodeID node, std::vector<uint32_t>& out) {
        if (!scene.IsValid(node)) { return; }
        if (const MeshProperty* mesh = scene.GetProperty<MeshProperty>(node)) {
            out.insert(out.end(), mesh->materials.begin(), mesh->materials.end());
        }
        for (const NodeID child : scene.GetChildren(node)) {
            AppendSubtreeMaterials(scene, child, out);
        }
    }

    [[nodiscard]] inline std::vector<uint32_t> CollectSubtreeMaterials(const ShiftScene& scene, NodeID node) {
        std::vector<uint32_t> used;
        AppendSubtreeMaterials(scene, node, used);
        std::sort(used.begin(), used.end());
        used.erase(std::unique(used.begin(), used.end()), used.end());
        return used;
    }

    class MaterialsPanel : public EditorPanel {
    public:
        using MaterialsProvider = std::function<Graphics::MaterialManager&()>;
        using SceneProvider = std::function<const ShiftScene&()>;

        MaterialsPanel(EditorContext& ctx, MaterialsProvider materials, SceneProvider scene)
            : EditorPanel("Materials"), m_context(ctx), m_materialsProvider(std::move(materials)),
              m_sceneProvider(std::move(scene)) {}

        void OnImGuiRender() override {
            if (!m_materialsProvider || !m_sceneProvider) { return; }
            Graphics::MaterialManager& materials = m_materialsProvider();

            ImGui::Checkbox("Selection only", &m_selectionOnly);
            ImGui::SameLine();
            ImGui::TextDisabled("(%u in scene)", materials.GetCount());
            ImGui::Separator();

            if (m_selectionOnly) {
                const ShiftScene& scene = m_sceneProvider();
                if (!scene.IsValid(m_context.selectedNode)) {
                    ImGui::TextDisabled("Nothing selected");
                    return;
                }
                const std::vector<uint32_t> used = CollectSubtreeMaterials(scene, m_context.selectedNode);
                if (used.empty()) {
                    ImGui::TextDisabled("The selection carries no meshes");
                    return;
                }
                for (const uint32_t index : used) { DrawMaterial(materials, index); }
                return;
            }

            for (uint32_t index = 0; index < materials.GetCount(); ++index) { DrawMaterial(materials, index); }
        }

    private:
        void DrawMaterial(Graphics::MaterialManager& materials, uint32_t index) {
            GPU::MaterialData* material = materials.EditMaterial(index);
            if (!material) { return; }

            ImGui::PushID(static_cast<int>(index));

            const std::string_view name = materials.GetName(index);
            const std::string_view shown = name.empty() ? std::string_view{"(unnamed)"} : name;
            if (ImGui::TreeNodeEx("material", ImGuiTreeNodeFlags_None, "#%u %.*s", index,
                                  static_cast<int>(shown.size()), shown.data())) {
                ImGui::ColorEdit4("Base color", &material->baseColorFactor.x, ImGuiColorEditFlags_Float);
                ImGui::ColorEdit3("Emissive", &material->emissiveFactor.x,
                                  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
                ImGui::SliderFloat("Metallic", &material->metallicFactor, 0.0f, 1.0f);
                ImGui::SliderFloat("Roughness", &material->roughnessFactor, 0.0f, 1.0f);

                const Graphics::EMaterialRasterFlags flags = materials.GetRasterFlags(index);
                bool masked = Graphics::Any(flags & Graphics::EMaterialRasterFlags::Masked);
                bool doubleSided = Graphics::Any(flags & Graphics::EMaterialRasterFlags::DoubleSided);
                bool flagsChanged = ImGui::Checkbox("Alpha tested", &masked);
                flagsChanged |= ImGui::Checkbox("Double sided", &doubleSided);
                if (flagsChanged) {
                    materials.SetRasterFlags(index, Graphics::MakeMaterialRasterFlags(masked, doubleSided));
                }

                ImGui::BeginDisabled(!masked);
                ImGui::SliderFloat("Alpha cutoff", &material->alphaCutoff, 0.0f, 1.0f);
                ImGui::EndDisabled();

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        EditorContext& m_context;
        MaterialsProvider m_materialsProvider;
        SceneProvider m_sceneProvider;

        bool m_selectionOnly = false;
    };
}

#endif //SHIFT_MATERIALSPANEL_HPP
