//
// Created by otrush on 1/26/2024.
//

#include "Swapchain.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace sft {
    namespace gfx {
        Swapchain::Swapchain(const Device &device, const WindowSurface& windowSurface, uint32_t width, uint32_t height):
        m_device{device}, m_windowSurface{windowSurface}
        {
            FillSwapchainDescription(width, height);
            CreateSwapChain();
            CreateImageViews();
        }

        void Swapchain::FillSwapchainDescription(uint32_t width, uint32_t height) {
            m_swapChainSupportDetails = sft::gutil::QuerySwapChainSupport(m_device.GetPhysicalDevice(), m_windowSurface.Get());

            m_swapchainDesc.surfaceFormat = gutil::ChooseSwapSurfaceFormat(m_swapChainSupportDetails.formats);
            m_swapchainDesc.presentMode = gutil::ChooseSwapPresentMode(m_swapChainSupportDetails.presentModes);
            m_swapchainDesc.swapChainImageFormat = m_swapchainDesc.surfaceFormat.format;
            m_swapchainDesc.swapChainExtent = gutil::ChooseSwapExtent(m_swapChainSupportDetails.capabilities, width, height);
        }

        void Swapchain::CreateSwapChain() {
            // This is basically getting the minimum amount of images for the swapchain, but +1 so we have a spare image
            uint32_t imageCount = m_swapChainSupportDetails.capabilities.minImageCount + 1;
            if (m_swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > m_swapChainSupportDetails.capabilities.maxImageCount) {
                imageCount = m_swapChainSupportDetails.capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = m_windowSurface.Get();
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = m_swapchainDesc.surfaceFormat.format;
            createInfo.imageColorSpace = m_swapchainDesc.surfaceFormat.colorSpace;
            createInfo.imageExtent = m_swapchainDesc.swapChainExtent;
            createInfo.imageArrayLayers = 1;
            // This parameter is used for render on screen, if you want ofscreen rendering for postprocess => VK_IMAGE_USAGE_TRANSFER_DST_BIT
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            sft::gutil::QueueFamilyIndices indices = sft::gutil::FindQueueFamilies(m_device.GetPhysicalDevice(), m_windowSurface.Get());
            std::array< uint32_t, 3 > queueFamilyIndices = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

            //! INFO: Basically we have queue families that may be different but work on the same image.
            //! This means that they need to share the resource, the simpler way is to have the access to the resource be concurrent.
            //! But you can make it exclusive, WHICH IS FASTER and explicitly transfer ownership from one family to another.
            if (indices.graphicsFamily != indices.presentFamily) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
                createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
            }
            else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }
            // We use default transform(do nothing with image)
            createInfo.preTransform = m_swapChainSupportDetails.capabilities.currentTransform;
            // This is for blending with other windows, FUCKING HELL
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = m_swapchainDesc.presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (int res = vkCreateSwapchainKHR(m_device.Get(), &createInfo, nullptr, &m_swapChain); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkImageView! Code: {:d}", res);
                return;
            }

            // Since we specify the minimum number of images in swapchain, vulkan can query more, hence this code
            vkGetSwapchainImagesKHR(m_device.Get(), m_swapChain, &imageCount, nullptr);
            m_swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(m_device.Get(), m_swapChain, &imageCount, m_swapChainImages.data());
        }

        void Swapchain::CreateImageViews() {
            m_swapChainImageViews.resize(m_swapChainImages.size());
            for (size_t i = 0; i < m_swapChainImages.size(); i++) {
                m_swapChainImageViews[i] = m_device.CreateImageView(
                        info::CreateImageViewInfo(m_swapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, m_swapchainDesc.swapChainImageFormat)
                        );
            }
        }

        bool Swapchain::IsValid() const {
            return m_swapChain != VK_NULL_HANDLE &&
                std::all_of(m_swapChainImageViews.begin(), m_swapChainImageViews.end(),
                            [](VkImageView view)
                {
                    return view != VK_NULL_HANDLE;
                });
        }

        Swapchain::~Swapchain() {
            for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
                vkDestroyImageView(m_device.Get(), m_swapChainImageViews[i], nullptr);
            }

            vkDestroySwapchainKHR(m_device.Get(), m_swapChain, nullptr);
        }
    }
} // sft