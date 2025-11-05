#ifndef SHIFT_VKUTILCORE_HPP
#define SHIFT_VKUTILCORE_HPP

#include "GLFW/glfw3.h"

#include <vector>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>

#include "Utility/Logging/LogMacros.hpp"

namespace Shift::VK::Util {
    const std::vector<const char*> VALIDATION_LAYERS = {
            "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> DEVICE_EXTENSIONS = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
            VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME
    };

    static constexpr float DEFAULT_QUEUE_PRIORITY = 1.0f;

    //! This struct stores the resepctive queues for graphics, presentation and more
    //! INFO: I use separate queue family for transfers just to practice Vulkan here, each graphics queue
    //! has VK_QUEUE_GRAPHICS_BIT!
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;

        [[nodiscard]] bool isComplete() {
            return !formats.empty() && !presentModes.empty();
        }
    };

    //! Log all the available validation layers
    //! \return True if all validation layers are supported
    bool CheckValidationLayerSupport();

    //! vkCreateDebugUtilsMessengerEXT is an extension function and we should load it before usage
    //! Returns nullptr if could not be loaded, else creates a DebugEXT object
    VkResult CreateDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator,
            VkDebugUtilsMessengerEXT* pDebugMessenger);

    //! Function for debug handle destruction should be loaded too
    void DestroyDebugUtilsMessengerEXT(
            VkInstance instance,
            VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks* pAllocator);

    //! Fill the needed debug messanger info, specify the layers
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

    //! Get the extensions required for Vulkan Instance
    std::vector<const char*> GetRequiredExtensions();

    //! Checks whether required GLFW extension is present in Vulkan and prints the info
    //! For now is stupid and expensive but is supposed to run at boot either way
    bool CheckForGLFWExtensionPresense(
            const char** glfwExtensions,
            uint32_t glfwExtensionCount,
            const std::vector<VkExtensionProperties>& vkExtensions);

    //! DEbug log all available extensions
    void PrintAvailibleVkExtensions(const std::vector<VkExtensionProperties>& vkExtensions);

    void PrintAvailablePhysicalDevices(const std::vector<VkPhysicalDevice>& devices);

    void PrintDeviceName(VkPhysicalDevice device, std::string prefix = "");

    // TODO: Location of these functions is sus
    //! This function rates the GPU by what features it supports, for now only looks and whether it is discrete, can run graphics commands, etc.
    int RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface);
    //! Here we just find the queue that supports graphics commands
    //! TODO: You can add logic to prefer a single queue family that supports the most features to increase performance
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    //! Check is all the device extensiona from the vector are supported
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    //! UTILITY
    //! Choose the surface format that you want
    //! TODO: Can make this function rank the formats and choose the best one
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    //! UTILITY
    //! Choose the swapchain present format(double or triple buffering)
    //! TODO: Specific and can be modified
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    //! UTILITY
    //! Couple of things here: if the capabilities are not the float limit - use them
    //! If they are, get the glfw frame buffer size and clamp the value to min or max possible range
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t winWidth, uint32_t winHeight);

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
} // Shift::VK::Util

#endif //SHIFT_VKUTILCORE_HPP
