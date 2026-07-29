#ifndef SHIFT_VKUTILCORE_HPP
#define SHIFT_VKUTILCORE_HPP

#include <vector>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <optional>
#include <span>

#include "Utility/Logging/LogMacros.hpp"
#include "Utility/Vulkan/VKInclude.hpp"
#include "Graphics/RHI/Common/Capabilities.hpp"

namespace Shift::VK::Util {
    extern const std::vector<const char*> VALIDATION_LAYERS;
    extern const float DEFAULT_QUEUE_PRIORITY;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> computeFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value()
                   && transferFamily.has_value() && computeFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;

        [[nodiscard]] bool isComplete() const {
            return !formats.empty() && !presentModes.empty();
        }
    };

    // Debug utils
    bool CheckValidationLayerSupport();
    //! Fills the messenger create-info with the engine's counting/logging callback
    //! The callback itself is internal to VKUtilCore.cpp it feeds the process-wide validation
    //! sink read through Shift::GetValidationStats().
    void FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    //! Build required device extension list from RHIRequiredFeatures
    std::vector<const char*> ResolveDeviceExtensions(const RHIRequiredFeatures& required);

    //! Instance-level required extensions (GLFW + optional debug)
    std::vector<const char*> GetRequiredInstanceExtensions(const RHIRequiredFeatures& required);

    //! Check GLFW extensions presence helper (keeps behavior)
    bool CheckForGLFWExtensionPresense(const char** glfwExtensions, uint32_t glfwExtensionCount, const std::vector<VkExtensionProperties>& vkExtensions);
    void PrintAvailibleVkExtensions(const std::vector<VkExtensionProperties>& vkExtensions);

    //! Physical device utilities - now accept required capabilities as argument where needed
    void PrintAvailablePhysicalDevices(const std::vector<VkPhysicalDevice>& devices);
    void PrintDeviceName(VkPhysicalDevice device, std::string prefix = "");

    //! Rate & pick devices
    int RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface, const RHIRequiredFeatures& required);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& desiredExtensions);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    //! Queue family policy, as a pure function of the driver-reported tables.
    //! Split out from FindQueueFamilies so it is testable without a device or a surface
    //! \param families queue family properties, indexed by family
    //! \param presentSupport per-family present capability, parallel to `families`
    //! \param forceUnified collapse everything onto the graphics family where legal
    QueueFamilyIndices SelectQueueFamilies(std::span<const VkQueueFamilyProperties> families,
                                           std::span<const VkBool32> presentSupport,
                                           bool forceUnified);

    //! Queue family finder: queries the device tables, then applies SelectQueueFamilies
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool forceUnified = false);

    //! Log the resolved family table plus any capability caveats
    void LogQueueFamilySelection(VkPhysicalDevice device, const QueueFamilyIndices& indices);

    //! Utilities for swapchain selection
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t winWidth, uint32_t winHeight);

    //! Format helper
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);

} // Shift::VK::Util

#endif //SHIFT_VKUTILCORE_HPP
