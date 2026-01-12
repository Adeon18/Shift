//
// Created by otrush on 1/12/2026.
//

#ifndef SHIFT_GENERICPANEL_HPP
#define SHIFT_GENERICPANEL_HPP

#include "EditorPanel.hpp"

namespace Shift::Editor {
    class GenericPanel : public EditorPanel {
    public:
        GenericPanel(const std::string& name) : EditorPanel(name) {}

        void OnImGuiRender() override {
            ImGui::Text("This is the %s panel :)", GetTitle().c_str());
        }
    };
}

#endif //SHIFT_GENERICPANEL_HPP