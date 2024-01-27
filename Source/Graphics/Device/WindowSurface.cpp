#include <Utility/Exceptions/VulkanExceptions.hpp>

#include "WindowSurface.hpp"

namespace sft {
    namespace gfx {
        WindowSurface::WindowSurface(const VkInstance ins, GLFWwindow *win): m_instance{ins} {
            if (const auto result = glfwCreateWindowSurface(ins, win, nullptr, &m_surface); result != VK_SUCCESS) {
                throw VulkanCreateResourceException("Error: Failed to create WindowSurface");
            }
        }

        WindowSurface::~WindowSurface() {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        };
    } // gfx
} // sft