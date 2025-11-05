#include "VKWindowSurface.hpp"

#include "VKMacros.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::VK {
    bool WindowSurface::Init(VkInstance ins, GLFWwindow *win) {
        m_instance = ins;
        if ( VkCheck(glfwCreateWindowSurface(ins, win, nullptr, &m_surface)) ) {
            Log(Critical, "Failed to create WindowSurface");
            return false;
        }
        return true;
    }

    void WindowSurface::Destroy() {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    };
} // Shift::VK