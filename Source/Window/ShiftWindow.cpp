#include "ShiftWindow.hpp"

#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

namespace Shift {

    void ShiftWindow::InitWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Because default is OpenGL so no API here
        //! Hints are global GLFW state: set the value both ways so one hidden window can never
        //! leak invisibility into a later window creation
        glfwWindowHint(GLFW_VISIBLE, m_visible ? GLFW_TRUE : GLFW_FALSE);

        m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
        glfwSetKeyCallback(m_window, KeyCallback);
        glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
        glfwSetCursorPosCallback(m_window, MouseCursorCallback);
        glfwSetScrollCallback(m_window, MouseScrollCallback);
    }

    ShiftWindow::ShiftWindow(uint32_t width, uint32_t height,  std::string  name, bool visible):
        m_width{width}, m_height{height}, m_name{std::move(name)}, m_visible{visible}
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
        // while (width == 0 || height == 0) {
        //     glfwGetFramebufferSize(m_window, &width, &height);
        //     glfwWaitEvents();
        // }

        m_width = width;
        m_height = height;
    }

    NativeWindow ShiftWindow::GetNativeWindow() const {
#ifdef _WIN32
        return NativeWindow{
            .platform = EPlatform::Win32,
            .window = glfwGetWin32Window(m_window),
            .instance = GetModuleHandleW(nullptr),
        };
#else
        return {};
#endif
    }

    ShiftWindow::~ShiftWindow() {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}