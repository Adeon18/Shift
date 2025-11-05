// //
// // Created by otrush on 3/19/2024.
// //
//
// #ifndef SHIFT_UICOMPONENT_HPP
// #define SHIFT_UICOMPONENT_HPP
//
// #include <string>
// #include <utility>
// #include <glm/gtc/type_ptr.hpp>
//
// #include "imgui/imgui.h"
// #include "imgui/imgui_internal.h"
//
// namespace Shift::gfx::ui {
//     class UIWindowComponent {
//     public:
//         explicit UIWindowComponent(std::string winName, std::string menuSectionName);
//         virtual void Item() = 0;
//
//         virtual void Show(uint32_t currentFrame) = 0;
//
//     protected:
//         std::string m_name;
//         std::string m_sectionName;
//         bool m_shown = false;
//     };
// }
//
// #endif //SHIFT_UICOMPONENT_HPP
