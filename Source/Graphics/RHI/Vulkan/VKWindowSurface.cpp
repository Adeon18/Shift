#include "VKWindowSurface.hpp"

#include "VKMacros.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::VK {
    WindowSurface::WindowSurface(VkInstance ins, GLFWwindow *win): m_instance(ins) {
        if ( VkCheck(glfwCreateWindowSurface(ins, win, nullptr, &m_surface)) ) {
            Log(Critical, "Failed to create WindowSurface");
            return;
        }
        m_valid = true;
    }

     WindowSurface::~WindowSurface() {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    };
} // Shift::VK