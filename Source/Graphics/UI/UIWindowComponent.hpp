//
// Created by otrush on 3/19/2024.
//

#ifndef SHIFT_UICOMPONENT_HPP
#define SHIFT_UICOMPONENT_HPP

#include <string>
#include <utility>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace shift::gfx::ui {
    class UIToolComponent {
    public:
        explicit UIToolComponent(std::string winName): m_name{std::move(winName)} {}

        virtual void Item() = 0
        {
            ImGui::MenuItem(m_name.c_str(), NULL, &m_shown);
        }

        virtual void Show() = 0;

    protected:
        std::string m_name;
        bool m_shown = false;
    };

    class WLightManager {
    public:
        WLightManager(const WLightManager&) = delete;
        WLightManager& operator=(const WLightManager&) = delete;

        static WLightManager& GetInstance() {
            static WLightManager u;
            return u;
        }

        void Item() {
            ImGui::MenuItem("ASS", NULL, &m_isOpen);
        }

        void Show() {

        }
    private:
        WLightManager() {}



        bool m_isOpen;
    };
}

#endif //SHIFT_UICOMPONENT_HPP
