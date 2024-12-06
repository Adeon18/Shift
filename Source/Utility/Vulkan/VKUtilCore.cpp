#include "VKUtilCore.hpp"

namespace Shift::VK::Util {
        bool CheckValidationLayerSupport() {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            Log(Trace, "Validation layer support:");

            // Check if all validation layers exist
            bool allLayersSupported = true;
            for (const char* layerName : VALIDATION_LAYERS) {
                bool layerFound = false;
                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }
                Log(Trace, "-> " + std::string{layerName} + ((layerFound) ? ": Availible" : ": Not Availible"));
                if (!layerFound) { allLayersSupported = false; }
            }

            return allLayersSupported;
        }

        VkResult CreateDebugUtilsMessengerEXT(
                VkInstance instance,
                const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkDebugUtilsMessengerEXT* pDebugMessenger)
        {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr) {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
            else {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        }

        void DestroyDebugUtilsMessengerEXT(
                VkInstance instance,
                VkDebugUtilsMessengerEXT debugMessenger,
                const VkAllocationCallbacks* pAllocator)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debugMessenger, pAllocator);
            }
        }

        void FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;
        }

        std::vector<const char*> GetRequiredExtensions() {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            // Check whether the extensions are avalible in vulkan
            uint32_t vkExtensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
            std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());
            if (!CheckForGLFWExtensionPresense(glfwExtensions, glfwExtensionCount, vkExtensions)) {
                throw std::runtime_error("Needed GLFW Extensions are not supported by Vulkan!");
            }
            PrintAvailibleVkExtensions(vkExtensions);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if SHIFT_VALIDATION
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

            return extensions;
        }

        //! Checks whether required GLFW extension is present in Vulkan and prints the info
        //! For now is stupid and expensive but is supposed to run at boot either way
        bool CheckForGLFWExtensionPresense(const char** glfwExtensions, uint32_t glfwExtensionCount, const std::vector<VkExtensionProperties>& vkExtensions) {
            Log(Trace, "GLFW extension support:");
            bool allExtSupported = true;
            for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
                bool present = false;
                for (auto& ext : vkExtensions) {
                    if (strcmp(ext.extensionName, glfwExtensions[i]) == 0) {
                        present = true;
                        break;
                    }
                }
                Log(Trace, "-> " + std::string{glfwExtensions[i]} + ((present) ? ": Supported" : ": Not Supported"));
                if (!present) { allExtSupported = false; }
            }

            return allExtSupported;
        }

        void PrintAvailibleVkExtensions(const std::vector<VkExtensionProperties>& vkExtensions) {
            Log(Trace, "Available Vk extensions:");
            for (const auto& extension : vkExtensions) {
                Log(Trace, "-> " + std::string{extension.extensionName});
            }
        }

        void PrintAvailablePhysicalDevices(const std::vector<VkPhysicalDevice>& devices) {
            Log(Trace, "Available Devices: ");
            for (const auto& device : devices) {
                PrintDeviceName(device, "-> ");
            }
        }

        void PrintDeviceName(VkPhysicalDevice device, std::string prefix) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            Log(Trace, prefix + deviceProperties.deviceName);
        }

        //! This function rates the GPU by what features it supports, for now only looks and whether it is discrete, can run graphics commands, etc.
        int RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface) {

            VkPhysicalDeviceProperties deviceProperties;    // Name/type/supported vk version
            VkPhysicalDeviceFeatures deviceFeatures;        // Texture size, shaders supported/etc.
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            int score = 0;

            // Discrete GPUs have a significant performance advantage
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 2;
            } else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                score += 1;
            }

            // TODO: Move this to separate function and add better device rating system
            if (!FindQueueFamilies(device, surface).isComplete()) {
                return 0;
            }

            if (!CheckDeviceExtensionSupport(device)) {
                return 0;
            }

            if (!QuerySwapChainSupport(device, surface).isComplete()) {
                return 0;
            }

            return score;
        }

        //! Here we just find the queue that supports graphics commands
        //! TODO: You can add logic to prefer a single queue family that supports the most features to increase performance
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
            QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            // Find at least one queue that supports graphics commands
            int i = 0;
            for (const auto& queueFamily : queueFamilies) {
                if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    indices.transferFamily = i++;
                    continue;
                }

                if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    indices.graphicsFamily = i;
                }
                // Fill presentation queue
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport) {
                    indices.presentFamily = i;
                }
                if (indices.isComplete()) {
                    break;
                }
                ++i;
            }

            if (!indices.transferFamily.has_value()) {
                indices.transferFamily = indices.graphicsFamily;
            }
            return indices;
        }

        bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }

        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
            SwapChainSupportDetails details;
            // Get surface capabilities
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

            // Get Surface formats
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
            if (formatCount != 0) {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
            }
            // Get present modes
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0) {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
            }

            return details;
        }

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }

        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t winWidth, uint32_t winHeight) {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
                return capabilities.currentExtent;
            }
            else {
                VkExtent2D actualExtent = {
                        winWidth,
                        winHeight
                };

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }

        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice) {
            for (VkFormat format : candidates) {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

                if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                    return format;
                } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                    return format;
                }
            }

            return VkFormat::VK_FORMAT_UNDEFINED;
        }
} // Shift::VK::Util