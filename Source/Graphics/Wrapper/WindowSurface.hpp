#ifndef SHIFT_WINDOWSURFACE_H
#define SHIFT_WINDOWSURFACE_H

#include <GLFW/glfw3.h>

namespace sft {
    namespace gfx {
        class WindowSurface {
        public:
            WindowSurface(const VkInstance ins, GLFWwindow* win);

            WindowSurface()=delete;
            WindowSurface(const WindowSurface&) = delete;
            WindowSurface& operator=(const WindowSurface&) = delete;

            [[nodiscard]] VkSurfaceKHR Get() const { return m_surface; }

            ~WindowSurface();
        private:

            VkInstance m_instance;
            VkSurfaceKHR m_surface;
        };
    } // gfx
} // sft

#endif //SHIFT_WINDOWSURFACE_H
