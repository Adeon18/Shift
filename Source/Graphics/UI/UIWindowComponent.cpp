// #include "UIWindowComponent.hpp"
//
// #include "UIManager.hpp"
//
// namespace Shift::gfx::ui {
//     UIWindowComponent::UIWindowComponent(std::string winName, std::string menuSectionName): m_name{std::move(winName)}, m_sectionName{menuSectionName} {
//         UIManager::GetInstance().RegisterToolComponent(this, menuSectionName);
//     }
//
//     void UIWindowComponent::Item() {
//         ImGui::MenuItem(m_name.c_str(), NULL, &m_shown);
//     }
// }