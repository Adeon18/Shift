#ifndef SHIFT_VKWINDOWSURFACE_H
#define SHIFT_VKWINDOWSURFACE_H

#include "GLFW/glfw3.h"

namespace Shift::VK {
    class WindowSurface {
    public:
        bool Init(VkInstance ins, GLFWwindow* win);

        WindowSurface()=default;
        WindowSurface(const WindowSurface&) = delete;
        WindowSurface& operator=(const WindowSurface&) = delete;

        [[nodiscard]] VkSurfaceKHR Get() const { return m_surface; }

        void Destroy();
        ~WindowSurface() = default;
    private:
        VkInstance m_instance;
        VkSurfaceKHR m_surface;
    };
} // Shift::VK

#endif //SHIFT_VKWINDOWSURFACE_H
