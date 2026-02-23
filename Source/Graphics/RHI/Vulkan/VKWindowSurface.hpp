#ifndef SHIFT_VKWINDOWSURFACE_H
#define SHIFT_VKWINDOWSURFACE_H

#include "Utility/Vulkan/VKInclude.hpp"
#include <GLFW/glfw3.h>

namespace Shift::VK {
    class WindowSurface {
    public:
        WindowSurface(VkInstance ins, GLFWwindow* win);
        WindowSurface(const WindowSurface&) = delete;
        WindowSurface& operator=(const WindowSurface&) = delete;

        [[nodiscard]] VkSurfaceKHR Get() const { return m_surface; }

        [[nodiscard]] bool IsValid() const { return m_valid; }

        ~WindowSurface();
    private:
        bool m_valid = false;
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    };
} // Shift::VK

#endif //SHIFT_VKWINDOWSURFACE_H
