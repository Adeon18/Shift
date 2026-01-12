#ifndef SHIFT_EDITORPANEL_H
#define SHIFT_EDITORPANEL_H

#include <string>
#include "imgui/imgui.h"

namespace Shift::Editor {

    class EditorPanel {
    protected:
        std::string m_Title;
        bool m_isOpen = true;
        bool m_isVisible = true;

    public:
        EditorPanel(const std::string& title) : m_Title(title) {}
        virtual ~EditorPanel() = default;

        virtual void OnImGuiRender() = 0;

        [[nodiscard]] const std::string& GetTitle() const { return m_Title; }
        bool& IsOpen() { return m_isOpen; }
        bool& IsVisible() { return m_isVisible; }
    };
}



#endif //SHIFT_EDITORPANEL_H