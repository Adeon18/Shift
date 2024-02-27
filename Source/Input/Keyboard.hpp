#ifndef SHIFT_KEYBOARD_HPP
#define SHIFT_KEYBOARD_HPP

#include <GLFW/glfw3.h>

#include <iostream>
#include <unordered_map>

namespace shift::inp {
    class Keyboard {
    public:
        static Keyboard &GetInstance() {
            static Keyboard inst;
            return inst;
        }

        Keyboard &operator=(const Keyboard &key) = delete;

        Keyboard(const Keyboard &key) = delete;

        bool IsJustPressed(int key) {
            return m_keyStatus[key] == GLFW_PRESS &&
                   m_keyStatusPrev[key] == GLFW_RELEASE;
        }

        bool IsJustReleased(int key) {
            return m_keyStatus[key] == GLFW_RELEASE &&
                   m_keyStatusPrev[key] == GLFW_PRESS;
        }

        bool IsPressed(int key) { return IsJustPressed(key) || m_keyStatus[key]; }

        bool IsReleased(int key) { return !m_keyStatus[key]; }

        int GetKeyAction(int key) { return m_keyStatus[key]; }

        void SetKeyAction(int key, int action) {
            m_keyStatus[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
        }

        void UpdateKeys() { m_keyStatusPrev = m_keyStatus; }

    private:
        Keyboard() {}

        std::unordered_map<int, bool> m_keyStatus{};
        std::unordered_map<int, bool> m_keyStatusPrev{};
    };

} // shift::inp
#endif //SHIFT_KEYBOARD_HPP
