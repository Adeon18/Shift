#include "ShiftWindow.hpp"

#include <utility>

namespace shift {

    void ShiftWindow::InitWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Because default is OpenGL so no API here

        m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
        glfwSetKeyCallback(m_window, KeyCallback);
        glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
        glfwSetCursorPosCallback(m_window, MouseCursorCallback);
        //SetCaptureCursor(true);
    }

    ShiftWindow::ShiftWindow(uint32_t width, uint32_t height,  std::string  name):
        m_width{width}, m_height{height}, m_name{std::move(name)}
    {
        InitWindow();
    }

    bool ShiftWindow::IsActive() {
        return !glfwWindowShouldClose(m_window);
    }

    void ShiftWindow::Process() {
        glfwPollEvents();

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        m_width = width;
        m_height = height;
    }

    ShiftWindow::~ShiftWindow() {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}