//
// Created by otrush on 1/7/2026.
//

#ifndef SHIFT_VIEWPORTPANEL_HPP
#define SHIFT_VIEWPORTPANEL_HPP

#include "imgui/imgui.h"
#include "EditorPanel.hpp"
#include "EditorContext.hpp"

namespace Shift::Editor {
    class ViewportPanel : public EditorPanel {
    private:
        EditorContext& m_Context;
        ImVec2 m_LastSize = {0, 0};
    public:
        ViewportPanel(EditorContext& ctx)
            : EditorPanel("Viewport"), m_Context(ctx) {}

        void ViewportPanel::OnImGuiRender() override {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImVec2 viewportSize = ImGui::GetContentRegionAvail();

            if (viewportSize.x > 0 && viewportSize.y > 0) {
                if (m_Context.EngineOutputTextureID) {
                    ImGui::Image(m_Context.EngineOutputTextureID, viewportSize);
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
    };
}

#endif //SHIFT_VIEWPORTPANEL_HPP