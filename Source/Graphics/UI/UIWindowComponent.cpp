#include "UIWindowComponent.hpp"

#include "UIManager.hpp"

namespace shift::gfx::ui {
    UIWindowComponent::UIWindowComponent(std::string winName, std::string menuSectionName): m_name{std::move(winName)}, m_sectionName{menuSectionName} {
        UIManager::GetInstance().RegisterToolComponent(this, menuSectionName);
    }
}