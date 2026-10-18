//
// Created by otrush on 9/15/2026.
//

#ifndef SHIFT_RENDERPANEL_HPP
#define SHIFT_RENDERPANEL_HPP

#include <array>
#include <functional>

#include "imgui/imgui.h"
#include "EditorPanel.hpp"

#include "Graphics/RendererSettings.hpp"

namespace Shift::Editor {
    class RenderPanel : public EditorPanel {
    public:
        using SettingsProvider = std::function<Graphics::RendererSettings&()>;

        explicit RenderPanel(SettingsProvider provider)
            : EditorPanel("Render"), m_provider(std::move(provider)) {}

        void OnImGuiRender() override {
            if (!m_provider) { return; }
            Graphics::RendererSettings& settings = m_provider();

            ImGui::SeparatorText("Tonemap");
            int currentOperator = static_cast<int>(settings.tonemapOperator);
            if (ImGui::Combo("Operator", &currentOperator, OPERATOR_NAMES.data(), static_cast<int>(OPERATOR_NAMES.size()))) {
                settings.tonemapOperator = static_cast<GPU::ETonemapOperator>(currentOperator);
            }

            ImGui::SliderFloat("Exposure (2^EV)", &settings.exposure, -20.0f, 8.0f, "%.2f");
            if (ImGui::Button("Reset exposure")) {
                settings.exposure = Graphics::RendererSettings::DEFAULT_EXPOSURE;
            }

            ImGui::SeparatorText("Debug view");
            int currentMode = static_cast<int>(settings.debugViewMode);
            if (ImGui::Combo("View", &currentMode, DEBUG_VIEW_NAMES.data(), static_cast<int>(DEBUG_VIEW_NAMES.size()))) {
                settings.debugViewMode = static_cast<GPU::EDebugViewMode>(currentMode);
            }

            ImGui::SeparatorText("Viewport overlays");
            ImGui::Checkbox("Light icons", &settings.showLightIcons);
            ImGui::SliderFloat("Icon size (px)", &settings.lightIconPixelSize, 6.0f, 64.0f, "%.0f");
            ImGui::ColorEdit3("Selection color", &settings.selectionColor.x, ImGuiColorEditFlags_Float);
            ImGui::SliderFloat("Selection brackets", &settings.selectionBracketFraction, 0.05f, 0.5f, "%.2f");
        }

    private:
        static constexpr std::array OPERATOR_NAMES{"None", "Reinhard", "Uncharted", "ACES"};
        static_assert(OPERATOR_NAMES.size() == static_cast<size_t>(GPU::ETonemapOperator::Count), "every ETonemapOperator needs a name in the combo");
        static constexpr std::array DEBUG_VIEW_NAMES{"Lit", "Albedo", "Normal", "Roughness", "Metallic", "Occlusion", "UV", "Mip"};
        static_assert(DEBUG_VIEW_NAMES.size() == static_cast<size_t>(GPU::EDebugViewMode::Count), "every EDebugViewMode needs a name in the combo");

        SettingsProvider m_provider;
    };
}

#endif //SHIFT_RENDERPANEL_HPP
