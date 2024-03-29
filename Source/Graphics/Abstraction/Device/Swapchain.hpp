#ifndef SHIFT_SWAPCHAIN_HPP
#define SHIFT_SWAPCHAIN_HPP

#include "Utility/Vulkan/UtilVulkan.hpp"

#include "Device.hpp"
#include "WindowSurface.hpp"

#include "Graphics/Abstraction/Synchronization/Semaphore.hpp"
#include "Graphics/Abstraction/Images/Images.hpp"

#include "Window/ShiftWindow.hpp"

namespace shift {
    namespace gfx {
        struct SwapchainDescription {
            VkSurfaceFormatKHR surfaceFormat;
            VkFormat swapChainImageFormat;
            VkExtent2D swapChainExtent;
            VkPresentModeKHR presentMode;
        };

        class Swapchain {
        public:
            Swapchain(const Device& device, const WindowSurface& windowSurface, uint32_t width, uint32_t height);
            Swapchain() = delete;
            Swapchain(const Swapchain&) = delete;
            Swapchain& operator=(const Swapchain&) = delete;

            [[nodiscard]] bool IsValid() const;

            //! Aquire next image index(with check for swapchain changed boolean)
            //! Returns the image index or UINT32_MAX if error
            [[nodiscard]] uint32_t AquireNextImageIndex(const Semaphore& semaphore, bool* wasChanged, uint64_t timeout = UINT64_MAX);
            [[nodiscard]] bool Recreate(uint32_t width, uint32_t height);
            [[nodiscard]] bool Present(const Semaphore& semaphore, uint32_t imageIdx, bool* isOld);

            [[nodiscard]] VkSwapchainKHR Get() const { return m_swapChain; }
            [[nodiscard]] const std::vector<VkImageView>& GetImageViews() const { return m_swapChainImageViews; }
            [[nodiscard]] const std::vector<VkImage>& GetImages() const { return m_swapChainImages; }
            [[nodiscard]] const SwapchainDescription& GetDesc() const { return m_swapchainDesc; }
            [[nodiscard]] VkExtent2D GetExtent() const { return m_swapchainDesc.swapChainExtent; }
            [[nodiscard]] VkFormat GetFormat() const { return m_swapchainDesc.swapChainImageFormat; }
            [[nodiscard]] VkViewport GetViewport() const { return m_viewPort; }
            [[nodiscard]] VkRect2D GetScissor() const { return m_scissor; }

            ~Swapchain();

        private:
            void FillSwapchainDescription(uint32_t width, uint32_t height);

            void CreateSwapChain();
            void CreateImageViews();

            void DestroyImageViews();

            const Device& m_device;
            const WindowSurface& m_windowSurface;

            VkViewport m_viewPort;
            VkRect2D m_scissor;

            VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

            SwapchainDescription m_swapchainDesc;

            gutil::SwapChainSupportDetails m_swapChainSupportDetails;

            std::vector<VkImage> m_swapChainImages;
            std::vector<VkImageView> m_swapChainImageViews;
        };
    } // gfx
} // shift

#endif //SHIFT_SWAPCHAIN_HPP
