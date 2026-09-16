//
// Created by otrush on 9/15/2026.
//

#ifndef SHIFT_TONEMAPPANEL_HPP
#define SHIFT_TONEMAPPANEL_HPP

#include <array>
#include <functional>

#include "imgui/imgui.h"
#include "EditorPanel.hpp"

#include "Graphics/RendererSettings.hpp"

//! Tonemap settings + exposure
namespace Shift::Editor {
    class TonemapPanel : public EditorPanel {
    public:
        using SettingsProvider = std::function<Graphics::RendererSettings&()>;

        explicit TonemapPanel(SettingsProvider provider)
            : EditorPanel("Tonemap"), m_provider(std::move(provider)) {}

        void OnImGuiRender() override {
            if (!m_provider) { return; }
            Graphics::RendererSettings& settings = m_provider();

            int current = static_cast<int>(settings.tonemapOperator);
            if (ImGui::Combo("Operator", &current, OPERATOR_NAMES.data(), static_cast<int>(OPERATOR_NAMES.size()))) {
                settings.tonemapOperator = static_cast<GPU::ETonemapOperator>(current);
            }

            ImGui::SliderFloat("Exposure (2^EV)", &settings.exposure, -8.0f, 8.0f, "%.2f");
            if (ImGui::Button("Reset exposure")) {
                settings.exposure = 0.0f;
            }
        }

    private:
        //! Same order as ETonemapOperator
        static constexpr std::array OPERATOR_NAMES{"None", "Reinhard", "Uncharted", "ACES"};
        static_assert(OPERATOR_NAMES.size() == static_cast<size_t>(GPU::ETonemapOperator::Count), "every ETonemapOperator needs a name in the combo");

        SettingsProvider m_provider;
    };
}

#endif //SHIFT_TONEMAPPANEL_HPP
