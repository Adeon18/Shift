#include "VKWindowSurface.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#include "VKMacros.hpp"
#include "Graphics/RHI/Common/Log.hpp"

namespace Shift::VK {
    WindowSurface::WindowSurface(VkInstance ins, const NativeWindow& window): m_instance(ins) {
        switch (window.platform) {
#ifdef _WIN32
            case EPlatform::Win32: {
                //! Volk can only load the Win32 surface function if VK_USE_PLATFORM_WIN32_KHR is defined for every file.
                //! With that define set, VMA's #include <vulkan/vulkan.h> pulls in windows.h everywhere, and its min/max/MemoryBarrier macros break the build
                //! So I do it by hand as otherwise - too much maintining and biolerplate
                const auto createSurface = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(ins, "vkCreateWin32SurfaceKHR"));
                if (createSurface == nullptr) {
                    SRHI_LOG(Critical, "vkCreateWin32SurfaceKHR is not available");
                    return;
                }

                VkWin32SurfaceCreateInfoKHR createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                createInfo.hinstance = static_cast<HINSTANCE>(window.instance);
                createInfo.hwnd = static_cast<HWND>(window.window);

                if (VkCheck(createSurface(ins, &createInfo, nullptr, &m_surface))) {
                    SRHI_LOG(Critical, "Failed to create WindowSurface");
                    return;
                }
                break;
            }
#endif
            default:
                SRHI_LOG(Critical, "No surface support for this platform");
                return;
        }
        m_valid = true;
    }

    std::vector<const char*> WindowSurface::GetInstanceExtensions(EPlatform platform) {
        switch (platform) {
#ifdef _WIN32
            case EPlatform::Win32: return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
#endif
            default: return {};
        }
    }

    WindowSurface::~WindowSurface() {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }
} // Shift::VK
