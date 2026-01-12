#include "VKUtilCore.hpp"

namespace Shift::VK::Util {
    const std::vector<const char*> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };

    const float DEFAULT_QUEUE_PRIORITY = 1.0f;

    //! ------------- Debug utils -------------
    bool CheckValidationLayerSupport() {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        bool allSupported = true;
        Log(Trace, "Checking Vulkan validation layer support...");

        for (const char* layerName : VALIDATION_LAYERS) {
            bool found = std::any_of(
                availableLayers.begin(), availableLayers.end(),
                [&](const VkLayerProperties& p) { return strcmp(p.layerName, layerName) == 0; }
            );

            Log(Trace, std::string(" -> ") + layerName + (found ? ": Available" : ": MISSING"));
            if (!found) allSupported = false;
        }
        return allSupported;
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
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void FillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
    }

    std::vector<const char *> ResolveDeviceExtensions(const RHIRequiredFeatures &required)
    {
        std::vector<const char*> result;

        // swapchain is still an extension
        if (required.VK_requireSwapchain) result.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        if (required.VK_presentWait) result.push_back(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
        if (required.VK_presentWait) result.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);
        if (required.VK_maintenance1) {
            result.push_back(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
        }

        //! TODO: [FEATURE] ADD OTHER EXTENSIONS HERE

        // Unique
        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());
        return result;
    }

    // Build instance extension list from GLFW plus debug if requested
    std::vector<const char*> GetRequiredInstanceExtensions(const RHIRequiredFeatures& required) {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (required.VK_enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        if (required.VK_maintenance1) {
            extensions.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
            extensions.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
        }

        // Validate presence
        uint32_t vkExtCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtCount, nullptr);
        std::vector<VkExtensionProperties> available(vkExtCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtCount, available.data());

        // If any required extension missing, throw (hard fail)
        for (const char* ext : extensions) {
            bool found = std::any_of(available.begin(), available.end(),
                                     [&](const VkExtensionProperties& p){ return strcmp(p.extensionName, ext) == 0; });
            if (!found) {
                throw std::runtime_error(std::string("Required instance extension missing: ") + ext);
            }
        }

        PrintAvailibleVkExtensions(available); // logging
        return extensions;
    }

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

    bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& desiredExtensions) {
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> available(extCount);
        if (extCount) vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, available.data());

        std::set<std::string> required(desiredExtensions.begin(), desiredExtensions.end());
        for (const auto& e : available) required.erase(e.extensionName);

        if (!required.empty()) {
            for (const auto& m : required) Log(Trace, std::string("Missing device extension: ") + m);
            return false;
        }
        return true;
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

        int i = 0;
        for (const auto& fam : families) {
            if (fam.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            if (fam.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                indices.computeFamily = i;
            }
            if (fam.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                indices.transferFamily = i;
            }
            VkBool32 presentSupport = false;
            if (surface != VK_NULL_HANDLE) {
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport)
                    indices.presentFamily = i;
            }
            ++i;
        }

        return indices;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        // Prefer SRGB + BGRA
        for (const auto& available : availableFormats) {
            if (available.format == VK_FORMAT_B8G8R8A8_SRGB && available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return available;
            }
        }
        // otherwise return first
        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // Prefer MAILBOX (triple buffering) if available
        for (const auto& mode : availablePresentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
        }
        // FIFO is always available
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t winWidth, uint32_t winHeight) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = { winWidth, winHeight };
            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
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
        throw std::runtime_error("Failed to find supported format!");
    }

    int RateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface, const RHIRequiredFeatures& required) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        int score = 0;
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) score += 1000;
        else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 500;

        // Queue checks
        if (!FindQueueFamilies(device, surface).isComplete()) return 0;

        // Device extensions
        if (!CheckDeviceExtensionSupport(device, ResolveDeviceExtensions(required))) return 0;

        // Swapchain support if requested
        if (required.VK_requireSwapchain) {
            if (!QuerySwapChainSupport(device, surface).isComplete()) return 0;
        }

        // Now query feature support for the requested features and hard-fail at selection time if missing.
        // We'll query a minimal set of features via features2 and its pNext chain.
        VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        VkPhysicalDeviceVulkan11Features vk11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
        VkPhysicalDeviceVulkan12Features vk12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        VkPhysicalDeviceVulkan13Features vk13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features2.pNext = &vk11;
        vk11.pNext = &vk12;
        vk12.pNext = &vk13;

        vkGetPhysicalDeviceFeatures2(device, &features2);

        // map requested -> supported checks
        auto isSupported = [&](const RHIRequiredFeatures& req)->bool {
            if (req.geometryShader && !features2.features.geometryShader) return false;
            if (req.tessellationShader && !features2.features.tessellationShader) return false;
            if (req.samplerAnisotropy && !features2.features.samplerAnisotropy) return false;
            if (req.VK_timelineSemaphore && !vk12.timelineSemaphore) return false;
            if (req.VK_descriptorIndexing && !vk12.descriptorIndexing) return false;
            if (req.VK_dynamicRendering && !(vk13.dynamicRendering)) return false;
            // if (req.VK_requireComputeQueue) {
            //     // require compute queue family
            //     auto q = FindQueueFamilies(device, surface);
            //     if (!q.computeFamily.has_value()) return false;
            // }
            // host query reset
            if (req.VK_hostQueryReset && !(vk12.hostQueryReset || /* or ext */ false)) {
                // Note: vk12.hostQueryReset is in VkPhysicalDeviceVulkan12Features (hostQueryReset)
                if (!vk12.hostQueryReset) return false;
            }
            // Additional checks (texture compression) could be added with format queries
            return true;
        };

        if (!isSupported(required)) return 0;

        // If passed all checks, rank by some extra heuristics
        score += (int)deviceProperties.limits.maxImageDimension2D / 1000;
        return score;
    }
} // Shift::VK::Util