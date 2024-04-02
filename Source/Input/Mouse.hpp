#ifndef SHIFT_MOUSE_H
#define SHIFT_MOUSE_H

#include "Input/Keyboard.hpp"
#include <GLFW/glfw3.h>
#include <utility>

namespace shift::inp {
    class Mouse {
    public:
        static Mouse &GetInstance() {
            static Mouse inst;
            return inst;
        }

        Mouse &operator=(const Mouse &key) = delete;
        Mouse(const Mouse &key) = delete;

        void SetPos(double xpos, double ypos) {
            m_xpos = xpos;
            m_ypos = ypos;
        }

        void UpdatePos() {
            m_prevXpos = m_xpos;
            m_prevYpos = m_ypos;

            m_xOffset = 0;
            m_yOffset = 0;
        }

        void SetScrollData(double xOffset, double yOffset) {
            m_xOffset = xOffset;
            m_yOffset = yOffset;
        }

        double GetXpos() { return m_xpos; }
        double GetYpos() { return m_ypos; }
        double GetXMovement() { return m_xpos - m_prevXpos; }
        double GetYMovement() { return m_ypos - m_prevYpos; }
        double GetYScroll() { return m_yOffset; }
        std::pair<double, double> GetMovement() {
            return {GetXMovement(), GetYMovement()};
        }
        std::pair<double, double> GetPos() { return {m_xpos, m_ypos}; }

        [[nodiscard]] bool isLeftButtonPressed() {
            return Keyboard::GetInstance().IsPressed(GLFW_MOUSE_BUTTON_LEFT);
        }

        [[nodiscard]] bool isLeftButtonJustPressed() {
            return Keyboard::GetInstance().IsJustPressed(GLFW_MOUSE_BUTTON_LEFT);
        }

        [[nodiscard]] bool isRightButtonPressed() {
            return Keyboard::GetInstance().IsPressed(GLFW_MOUSE_BUTTON_RIGHT);
        }

        [[nodiscard]] bool isRightButtonJustPressed() {
            return Keyboard::GetInstance().IsJustPressed(GLFW_MOUSE_BUTTON_RIGHT);
        }

        [[nodiscard]] bool isMiddleButtonPressed() {
            return Keyboard::GetInstance().IsPressed(GLFW_MOUSE_BUTTON_MIDDLE);
        }

        [[nodiscard]] bool isMiddleButtonJustPressed() {
            return Keyboard::GetInstance().IsJustPressed(GLFW_MOUSE_BUTTON_MIDDLE);
        }

    private:
        Mouse() {}

        double m_xpos, m_ypos;
        double m_prevXpos, m_prevYpos;
        double m_xOffset = 0.0f;
        double m_yOffset = 0.0f;
    };
} // shift::inp

#endif //SHIFT_MOUSE_H
