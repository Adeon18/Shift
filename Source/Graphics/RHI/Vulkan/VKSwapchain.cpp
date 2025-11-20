//
// Created by otrush on 1/26/2024.
//

#include "VKSwapchain.hpp"
#include <array>

#include "Utility/Assertions.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    bool Swapchain::Init(const Device *device, const WindowSurface* windowSurface, uint32_t width, uint32_t height) {
        m_device = device;
        m_windowSurface = windowSurface;
        FillSwapchainDescription(width, height);
        CheckCritical(CreateSwapChain(), "Failed to create swapchain!");
        CheckCritical(CreateImageViews(), "Failed to create image views!");

        return true;
    }

    void Swapchain::FillSwapchainDescription(uint32_t width, uint32_t height) {
        m_swapChainSupportDetails = Util::QuerySwapChainSupport(m_device->GetPhysicalDevice(), m_windowSurface->Get());

        m_swapchainDesc.surfaceFormat = Util::ChooseSwapSurfaceFormat(m_swapChainSupportDetails.formats);
        m_swapchainDesc.presentMode = Util::ChooseSwapPresentMode(m_swapChainSupportDetails.presentModes);
        m_swapchainDesc.swapChainImageFormat = Util::VKToShiftTextureFormat(m_swapchainDesc.surfaceFormat.format);
        auto extent = Util::ChooseSwapExtent(m_swapChainSupportDetails.capabilities, width, height);
        m_swapchainDesc.swapChainExtent = Extent2D{extent.width, extent.height};
    }

    bool Swapchain::CreateSwapChain() {
        // This is basically getting the minimum amount of images for the swapchain, but +1 so we have a spare image
        uint32_t imageCount = m_swapChainSupportDetails.capabilities.minImageCount + 1;
        if (m_swapChainSupportDetails.capabilities.maxImageCount > 0 && imageCount > m_swapChainSupportDetails.capabilities.maxImageCount) {
            imageCount = m_swapChainSupportDetails.capabilities.maxImageCount;
        }

        VkSwapchainKHR oldSwapchain = m_swapChain;

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_windowSurface->Get();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = m_swapchainDesc.surfaceFormat.format;
        createInfo.imageColorSpace = m_swapchainDesc.surfaceFormat.colorSpace;
        createInfo.imageExtent = VkExtent2D{m_swapchainDesc.swapChainExtent.x, m_swapchainDesc.swapChainExtent.y};
        createInfo.imageArrayLayers = 1;
        // This parameter is used for render on screen, if you want ofscreen rendering for postprocess => VK_IMAGE_USAGE_TRANSFER_DST_BIT
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        //! ONly 2 queues are needed max
        Util::QueueFamilyIndices indices = m_device->GetQueueFamilyIndices();
        uint32_t queueFamilyIndices[2];
        uint32_t uniqueFamilyCount = 0;

        queueFamilyIndices[uniqueFamilyCount++] = indices.graphicsFamily.value();

        if (indices.presentFamily.value() != indices.graphicsFamily.value()) {
            queueFamilyIndices[uniqueFamilyCount++] = indices.presentFamily.value();
        }

        if (uniqueFamilyCount > 1) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = uniqueFamilyCount;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        // We use default transform(do nothing with image)
        createInfo.preTransform = m_swapChainSupportDetails.capabilities.currentTransform;
        // This is for blending with other windows, FUCKING HELL
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = m_swapchainDesc.presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;

        if ( VkCheckV(vkCreateSwapchainKHR(m_device->Get(), &createInfo, nullptr, &m_swapChain), res) ) {
            Log(Critical, "Failed to create VkImageView!");
            return false;
        }

        if (oldSwapchain != VK_NULL_HANDLE) {
            DestroyImageViews();
            vkDestroySwapchainKHR(m_device->Get(), oldSwapchain, nullptr);
        }

        // Since we specify the minimum number of images in swapchain, vulkan can query more, hence this code
        m_swapChainImages.clear();
        vkGetSwapchainImagesKHR(m_device->Get(), m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device->Get(), m_swapChain, &imageCount, m_swapChainImages.data());

        m_viewPort.x = 0.0f;
        //! TODO: BUG???
        m_viewPort.y = static_cast<float>(m_swapchainDesc.swapChainExtent.y);
        m_viewPort.width = static_cast<float>(m_swapchainDesc.swapChainExtent.x);
        m_viewPort.height = -static_cast<float>(m_swapchainDesc.swapChainExtent.y);
        m_viewPort.minDepth = 0.0f;
        m_viewPort.maxDepth = 1.0f;

        m_scissor.offset = { 0, 0 };
        m_scissor.extent = m_swapchainDesc.swapChainExtent;

        return true;
    }

    bool Swapchain::CreateImageViews() {
        m_swapChainTextures.resize(m_swapChainImages.size());
        bool success = true;
        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            m_swapChainTextures[i].m_image = m_swapChainImages[i];
            m_swapChainTextures[i].m_imageView = m_device->CreateImageView(
                Util::CreateImageViewInfo(m_swapChainImages[i], VK_IMAGE_VIEW_TYPE_2D, Util::ShiftToVKTextureFormat(m_swapchainDesc.swapChainImageFormat))
            );
            CheckExit(m_swapChainTextures[i].m_imageView != VK_NULL_HANDLE);
            m_swapChainTextures[i].m_textureDesc.resourceLayout = Util::VKToShiftResourceLayout(VK_IMAGE_LAYOUT_UNDEFINED);
            m_swapChainTextures[i].m_stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
        return success;
    }

    uint32_t Swapchain::AquireNextImage(const BinarySemaphore& semaphore, bool* wasChanged, uint64_t timeout) {
        uint32_t imageIdx = 0;
        VkResult result = vkAcquireNextImageKHR(m_device->Get(), m_swapChain, timeout, semaphore.Get(), VK_NULL_HANDLE, &imageIdx);

        // Screen resize handling
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            *wasChanged = true;
            return imageIdx;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            Log(Error, "Failed to recreate swapchain! Code: {}", static_cast<int>(result));
            return UINT32_MAX;
        }

        return imageIdx;
    }

    bool Swapchain::Recreate(uint32_t width, uint32_t height) {
        vkDeviceWaitIdle(m_device->Get());
        FillSwapchainDescription(width, height);
        CheckCritical(CreateSwapChain(), "Failed to create swapchain!");
        CheckCritical(CreateImageViews(), "Failed to create image views!");
        Log(Info, "Recreated Swapchain");
        return true;
    }


    void Swapchain::DestroyImageViews() {
        for (size_t i = 0; i < m_swapChainTextures.size(); i++) {
            vkDestroyImageView(m_device->Get(), m_swapChainTextures[i].m_imageView, nullptr);
        }
    }

    void Swapchain::Destroy() {
        DestroyImageViews();

        vkDestroySwapchainKHR(m_device->Get(), m_swapChain, nullptr);
    }

    bool Swapchain::Present(const BinarySemaphore &semaphore, uint32_t imageIdx, bool* isOld) {
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

        VkResult result = vkQueuePresentKHR(m_device->GetPresentQueue(), &presentInfo);

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
} // Shift::VK