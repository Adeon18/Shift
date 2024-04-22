//
// Created by otrush on 1/26/2024.
//

#include "Swapchain.hpp"
#include <array>

#include "Utility/Vulkan/InfoUtil.hpp"

namespace shift {
    namespace gfx {
        Swapchain::Swapchain(const Device &device, const WindowSurface& windowSurface, uint32_t width, uint32_t height):
        m_device{device}, m_windowSurface{windowSurface}
        {
            FillSwapchainDescription(width, height);
            CreateSwapChain();
            CreateImageViews();
        }

        void Swapchain::FillSwapchainDescription(uint32_t width, uint32_t height) {
            m_swapChainSupportDetails = shift::gutil::QuerySwapChainSupport(m_device.GetPhysicalDevice(), m_windowSurface.Get());

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

            VkSwapchainKHR oldSwapchain = m_swapChain;

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

            shift::gutil::QueueFamilyIndices indices = shift::gutil::FindQueueFamilies(m_device.GetPhysicalDevice(), m_windowSurface.Get());
            std::array< uint32_t, 3 > queueFamilyIndices = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

            //! INFO: Basically we have queue families that may be different but work on the same image.
            //! This means that they need to share the resource, the simpler way is to have the access to the resource be concurrent.
            //! But you can make it exclusive, WHICH IS FASTER and explicitly transfer ownership from one family to another.
            if (indices.graphicsFamily != indices.presentFamily) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
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
            createInfo.oldSwapchain = oldSwapchain;

            if (int res = vkCreateSwapchainKHR(m_device.Get(), &createInfo, nullptr, &m_swapChain); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkImageView! Code: {:d}", res);
                return;
            }

            if (oldSwapchain != VK_NULL_HANDLE) {
                DestroyImageViews();
                vkDestroySwapchainKHR(m_device.Get(), oldSwapchain, nullptr);
            }

            // Since we specify the minimum number of images in swapchain, vulkan can query more, hence this code
            m_swapChainImages.clear();
            vkGetSwapchainImagesKHR(m_device.Get(), m_swapChain, &imageCount, nullptr);
            m_swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(m_device.Get(), m_swapChain, &imageCount, m_swapChainImages.data());

            m_viewPort.x = 0.0f;
            m_viewPort.y = static_cast<float>(m_swapchainDesc.swapChainExtent.height);
            m_viewPort.width = static_cast<float>(m_swapchainDesc.swapChainExtent.width);
            m_viewPort.height = -static_cast<float>(m_swapchainDesc.swapChainExtent.height);
            m_viewPort.minDepth = 0.0f;
            m_viewPort.maxDepth = 1.0f;

            m_scissor.offset = { 0, 0 };
            m_scissor.extent = m_swapchainDesc.swapChainExtent;
        }

        void Swapchain::CreateImageViews() {
            m_swapChainImageViews.resize(m_swapChainImages.size());
            for (size_t i = 0; i < m_swapChainImages.size(); i++) {
                m_swapChainImageViews[i] = m_device.CreateImageView(
                        info::CreateImageViewInfo(m_swapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, m_swapchainDesc.swapChainImageFormat)
                        );
            }
        }

        uint32_t Swapchain::AquireNextImageIndex(const Semaphore& semaphore, bool* wasChanged, uint64_t timeout) {
            uint32_t imageIdx = 0;
            VkResult result = vkAcquireNextImageKHR(m_device.Get(), m_swapChain, timeout, semaphore.Get(), VK_NULL_HANDLE, &imageIdx);

            // Screen resize handling
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                *wasChanged = true;
                return imageIdx;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                spdlog::error("Failed to recreate swapchain! Code: {}", static_cast<int>(result));
                return UINT32_MAX;
            }

            return imageIdx;
        }

        bool Swapchain::Recreate(uint32_t width, uint32_t height) {
            vkDeviceWaitIdle(m_device.Get());
            FillSwapchainDescription(width, height);
            CreateSwapChain();
            CreateImageViews();
            if (!IsValid()) {
                return false;
            }
            spdlog::info("Recreated Swapchain");
            return true;
        }

        bool Swapchain::IsValid() const {
            return m_swapChain != VK_NULL_HANDLE &&
                std::all_of(m_swapChainImageViews.begin(), m_swapChainImageViews.end(),
                            [](VkImageView view)
                {
                    return view != VK_NULL_HANDLE;
                });
        }

        void Swapchain::DestroyImageViews() {
            for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
                vkDestroyImageView(m_device.Get(), m_swapChainImageViews[i], nullptr);
            }
        }

        Swapchain::~Swapchain() {
            DestroyImageViews();

            vkDestroySwapchainKHR(m_device.Get(), m_swapChain, nullptr);
        }

        bool Swapchain::Present(const Semaphore &semaphore, uint32_t imageIdx, bool* isOld) {
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            VkSemaphore semaphores[] = {semaphore.Get()};
            presentInfo.pWaitSemaphores = semaphores;

            presentInfo.swapchainCount = 1;
            VkSwapchainKHR swapchains[] = {m_swapChain};
            presentInfo.pSwapchains = swapchains;
            presentInfo.pImageIndices = &imageIdx;

            presentInfo.pResults = nullptr; // Optional

            VkResult result = vkQueuePresentKHR(m_device.GetPresentQueue(), &presentInfo);

            // Screen resize handling
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                *isOld = true;
                return true;
            }
            else if (result != VK_SUCCESS) {
                spdlog::error("Failed to recreate swapchain! Code: {}", static_cast<int>(result));
                return false;
            }
            return true;
        }
    }
} // shift