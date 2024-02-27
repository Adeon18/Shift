#ifndef SHIFT_WINDOW_HPP
#define SHIFT_WINDOW_HPP

#include <GLFW/glfw3.h>

#include <string>

#include "Input/Mouse.hpp"

namespace shift {
	class ShiftWindow {
    public:
		ShiftWindow(uint32_t width, uint32_t height, std::string name);
        ShiftWindow()=delete;
		ShiftWindow(const ShiftWindow&)=delete;
		ShiftWindow& operator=(const ShiftWindow&)=delete;

        //! Check whether the window is not requested to close
		[[nodiscard]] bool IsActive();
        //! Poll events, handle minimization
        void Process();

        //! Window resize callback
        static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
            auto app = reinterpret_cast<ShiftWindow*>(glfwGetWindowUserPointer(window));
            app->m_shoudProcessResize = true;
        }

        static void KeyCallback(GLFWwindow *window, int key, int scancode,
                                     int action, int mods) {
            inp::Keyboard &keyboard = inp::Keyboard::GetInstance();
            keyboard.SetKeyAction(key, action);
        }

        static void MouseButtonCallback(GLFWwindow *window, int button, int action,
                                             int mods) {
            inp::Keyboard &keyboard = inp::Keyboard::GetInstance();
            keyboard.SetKeyAction(button, action);
        }

        static void MouseCursorCallback(GLFWwindow *window, double xpos,
                                             double ypos) {
            inp::Mouse &mouse = inp::Mouse::GetInstance();
            mouse.SetPos(xpos, ypos);
        }

        void SetCaptureCursor(bool capture) {
            glfwSetInputMode(m_window, GLFW_CURSOR,
                             capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }

        //! Get whether the window was resized and we should process it
        [[nodiscard]] bool ShouldProcessResize() const { return m_shoudProcessResize; }
        //! Mark window resize as processed
        void ProcessResize() { m_shoudProcessResize = false; }

        [[nodiscard]] GLFWwindow* GetHandle() const { return m_window; }
        [[nodiscard]] uint32_t GetWidth() const { return m_width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_height; }

		~ShiftWindow();
	private:
		void InitWindow();

		GLFWwindow* m_window = nullptr;
        std::string m_name;
		uint32_t m_width;
		uint32_t m_height;

        // True when was resized but was not processed by the engine
        bool m_shoudProcessResize = false;
	};
}

#endif //SHIFT_WINDOW_HPP