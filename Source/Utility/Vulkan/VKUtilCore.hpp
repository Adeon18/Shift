#ifndef SHIFT_VKUTILCORE_HPP
#define SHIFT_VKUTILCORE_HPP

#include <vector>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>

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
    void FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    //! VKAPI_ATTR and VKAPI_ATTR ensure that Vulkan has the right signature to call the function
    //! Callback very similar to DirectX
    static VKAPI_ATTR VkBool32 VKAPI_ATTR debugCallback
            (
                    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     // Severity of the message, verbose < info < warning < error
                    VkDebugUtilsMessageTypeFlagsEXT messageType,                // Basically General, Validation or Performance
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  // Basically inportant stuff, like the message, object handles and size
                    void* pUserData                                             // Pointer that you can pass your own data to
            ) {
        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                Log(Trace, "Validation layer: " + std::string{pCallbackData->pMessage});
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                Log(Info, "Validation layer: " + std::string{pCallbackData->pMessage});
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                Log(Warn, "Validation layer: " + std::string{pCallbackData->pMessage});
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                Log(Error, "Validation layer: " + std::string{pCallbackData->pMessage});
                break;
        }

        return VK_FALSE;
    }

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

    //! Queue family finder
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    //! Utilities for swapchain selection
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t winWidth, uint32_t winHeight);

    //! Format helper
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);

} // Shift::VK::Util

#endif //SHIFT_VKUTILCORE_HPP
