#ifndef SHIFT_VKWINDOWSURFACE_H
#define SHIFT_VKWINDOWSURFACE_H

#include <vector>

#include "Utility/Vulkan/VKInclude.hpp"
#include "Graphics/RHI/Common/NativeWindow.hpp"

namespace Shift::VK {
    class WindowSurface {
    public:
        WindowSurface(VkInstance ins, const NativeWindow& window);
        WindowSurface(const WindowSurface&) = delete;
        WindowSurface& operator=(const WindowSurface&) = delete;

        //! Instance extensions a surface on this platform needs
        [[nodiscard]] static std::vector<const char*> GetInstanceExtensions(EPlatform platform);

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
