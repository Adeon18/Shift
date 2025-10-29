#ifndef SHIFT_VKSWAPCHAIN_HPP
#define SHIFT_VKSWAPCHAIN_HPP

#include "Utility/Vulkan/VKUtilCore.hpp"

#include "Graphics/RHI/Swapchain.hpp"
#include "VKDevice.hpp"
#include "VKWindowSurface.hpp"
#include "VKSemaphore.hpp"
#include "VKTexture.hpp"

#include "Window/ShiftWindow.hpp"

namespace Shift::VK {
    struct SwapchainDescription {
        VkSurfaceFormatKHR surfaceFormat;
        ETextureFormat swapChainImageFormat;
        Extent2D swapChainExtent;
        VkPresentModeKHR presentMode;
    };

    class Swapchain {
    public:
        Swapchain() = default;
        Swapchain(const Swapchain&) = delete;
        Swapchain& operator=(const Swapchain&) = delete;

        [[nodiscard]] bool Init(const Device* device, const WindowSurface* windowSurface, uint32_t width, uint32_t height);

        [[nodiscard]] bool IsValid() const;

        //! Aquire next image index(with check for swapchain changed boolean)
        //! \param semaphore Semaphore that is signalled when we get the image
        //! \param wasChanged boolean that is filled based on if the swapchain was successfully changed
        //! \param timeout max time to wait until success, default is UINT64_MAX
        //! \return The image index or UINT32_MAX if error
        [[nodiscard]] uint32_t AquireNextImage(const Semaphore& semaphore, bool* wasChanged, uint64_t timeout = UINT64_MAX);
        //! Wait for device commands and manually recreate the swapchain
        //! \param width
        //! \param height
        //! \return Whether recreation was a success
        [[nodiscard]] bool Recreate(uint32_t width, uint32_t height);
        //! Present everything to screen and bind a respective semaphore
        //! \param semaphore
        //! \param imageIdx The image index in the swapchain to present
        //! \param isOld Is filled when the semaphore is old and should be recreated
        //! \return false at total failure (no recreation possible), else true
        [[nodiscard]] bool Present(const Semaphore& semaphore, uint32_t imageIdx, bool* isOld);

        [[nodiscard]] VkSwapchainKHR Get() const { return m_swapChain; }
        [[nodiscard]] const std::vector<VkImageView>& GetImageViews() const { return m_swapChainImageViews; }
        [[nodiscard]] const std::vector<VkImage>& GetImages() const { return m_swapChainImages; }

        [[nodiscard]] Extent2D GetExtent() const { return m_swapchainDesc.swapChainExtent; }
        [[nodiscard]] ETextureFormat GetFormat() const { return m_swapchainDesc.swapChainImageFormat; }
        [[nodiscard]] Viewport GetViewport() const { return m_viewPort; }
        [[nodiscard]] Rect2D GetScissor() const { return m_scissor; }

        void Destroy();
        ~Swapchain()=default;

    private:
        void FillSwapchainDescription(uint32_t width, uint32_t height);

        bool CreateSwapChain();
        bool CreateImageViews();

        void DestroyImageViews();

        const Device* m_device = nullptr;
        const WindowSurface* m_windowSurface = nullptr;

        Viewport m_viewPort{};
        Rect2D m_scissor{};

        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;

        SwapchainDescription m_swapchainDesc{};

        Util::SwapChainSupportDetails m_swapChainSupportDetails;

        std::vector<VkImage> m_swapChainImages;
        std::vector<VkImageView> m_swapChainImageViews;
    };

    ASSERT_INTERFACE(ISwapchain, Swapchain);
} // Shift::VK

#endif //SHIFT_VKSWAPCHAIN_HPP
