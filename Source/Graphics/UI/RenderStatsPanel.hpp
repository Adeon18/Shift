//
// Created by otrush on 8/30/2026.
//

#ifndef SHIFT_RENDERSTATSPANEL_HPP
#define SHIFT_RENDERSTATSPANEL_HPP

#include <functional>

#include "imgui/imgui.h"
#include "EditorPanel.hpp"

#include "Graphics/RenderScene.hpp"

namespace Shift::Editor {
    //! Debug stat panel
    class RenderStatsPanel : public EditorPanel {
    public:
        using StatsProvider = std::function<const Graphics::RenderSceneStats&()>;

        explicit RenderStatsPanel(StatsProvider provider)
            : EditorPanel("Render Stats"), m_provider(std::move(provider)) {}

        void OnImGuiRender() override {
            static const Graphics::RenderSceneStats s_empty;
            const Graphics::RenderSceneStats& stats = m_provider ? m_provider() : s_empty;

            ImGui::Text("Placements  %u", stats.placements);
            ImGui::Text("Objects     %u", stats.objects);
            ImGui::Separator();
            ImGui::Text("Draw calls  %u", stats.drawCalls);
            ImGui::Text("Instances   %u", stats.instances);
            ImGui::Text("Culled      %u", stats.culled);

            if (stats.drawCalls == stats.instances) {
                ImGui::TextDisabled("(no instancing yet)");
            }
        }

    private:
        StatsProvider m_provider;
    };
}

#endif //SHIFT_RENDERSTATSPANEL_HPP
