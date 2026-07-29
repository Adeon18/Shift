#include "VKUtilCore.hpp"

#include <array>
#include <atomic>
#include <bit>
#include <mutex>

#include "GLFW/glfw3.h"

namespace Shift::VK::Util {
    const std::vector<const char*> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };

    const float DEFAULT_QUEUE_PRIORITY = 1.0f;

    //! Validation sink
    //! Static storage on purpose: teardown leak reports arrive while the RHI is mid-destruction,
    //! and tests assert on these counters AFTER the engine is gone.
    namespace {
        constexpr uint64_t VALIDATION_MESSAGE_RING_DEPTH = 8;

        std::atomic<uint64_t> s_validationErrors{0};
        std::atomic<uint64_t> s_validationWarnings{0};
        std::atomic<uint64_t> s_validationInfos{0};

        std::mutex s_validationRingMutex;
        std::array<std::string, VALIDATION_MESSAGE_RING_DEPTH> s_validationRing;
        uint64_t s_validationRingNext = 0;

        void RecordValidationMessage(const char* severity, const char* message) {
            std::lock_guard<std::mutex> guard(s_validationRingMutex);
            s_validationRing[s_validationRingNext % VALIDATION_MESSAGE_RING_DEPTH] =
                std::string("[") + severity + "] " + message;
            ++s_validationRingNext;
        }

        //! VKAPI_ATTR and VKAPI_CALL ensure that Vulkan has the right signature to call the function.
        //! Counts per severity into the sink above, then logs.
        VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback
                (
                        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,     // Severity of the message, verbose < info < warning < error
                        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,            // Basically General, Validation or Performance
                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,  // Basically inportant stuff, like the message, object handles and size
                        void* /*pUserData*/                                         // Pointer that you can pass your own data to
                ) {
            switch (messageSeverity) {
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                    Log(Trace, "Validation layer: {}", pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    s_validationInfos.fetch_add(1, std::memory_order_relaxed);
                    Log(Info, "Validation layer: {}", pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    s_validationWarnings.fetch_add(1, std::memory_order_relaxed);
                    RecordValidationMessage("warning", pCallbackData->pMessage);
                    Log(Warn, "Validation layer: {}", pCallbackData->pMessage);
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    s_validationErrors.fetch_add(1, std::memory_order_relaxed);
                    RecordValidationMessage("error", pCallbackData->pMessage);
                    Log(Error, "Validation layer: {}", pCallbackData->pMessage);
                    break;
                default:
                    break;
            }

            return VK_FALSE;
        }
    }

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
        createInfo.pfnUserCallback = DebugCallback;
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

    //! ========= Queue family selection ===========
    namespace {
        //! The spec guarantees that a family which has GRAPHICS or COMPUTE also supports
        //! transfer operations, whether or not VK_QUEUE_TRANSFER_BIT is actually set
        constexpr VkQueueFlags EffectiveQueueFlags(VkQueueFlags flags) {
            if (flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
                flags |= VK_QUEUE_TRANSFER_BIT;
            }
            return flags;
        }

        //! Pick the family that can do `wanted` while sharing as little else as possible.
        //! [Claude did this:0]
        //! Ranking, best first:
        //!   1. fewest of the `avoid` capabilities -- this is the rule that actually finds the
        //!      dedicated DMA family and the async-compute family. Ranking on the raw flags
        //!      value instead would rate a GRAPHICS|TRANSFER family (5) above a dedicated
        //!      TRANSFER|SPARSE one (12), since GRAPHICS is the cheapest bit
        //!   2. numerically smallest queueFlags -- among equally dedicated families the
        //!      high-valued engine bits (VIDEO_DECODE 0x20, VIDEO_ENCODE 0x40,
        //!      OPTICAL_FLOW 0x100) sort those families to the back for free
        //!   3. lowest family index
        std::optional<uint32_t> PickQueueFamily(std::span<const VkQueueFamilyProperties> families,
                                                VkQueueFlags wanted, VkQueueFlags avoid) {
            std::optional<uint32_t> best;
            int bestPenalty = 0;
            VkQueueFlags bestFlags = 0;

            for (uint32_t i = 0; i < families.size(); ++i) {
                if (families[i].queueCount == 0) continue;

                const VkQueueFlags flags = EffectiveQueueFlags(families[i].queueFlags);
                if ((flags & wanted) != wanted) continue;

                const int penalty = std::popcount(flags & avoid);
                const bool better = !best.has_value()
                                    || penalty < bestPenalty
                                    || (penalty == bestPenalty && flags < bestFlags);
                if (better) {
                    best = i;
                    bestPenalty = penalty;
                    bestFlags = flags;
                }
            }
            return best;
        }
    }

    QueueFamilyIndices SelectQueueFamilies(std::span<const VkQueueFamilyProperties> families,
                                           std::span<const VkBool32> presentSupport,
                                           bool forceUnified) {
        QueueFamilyIndices indices;

        //! Graphics: first capable family
        for (uint32_t i = 0; i < families.size(); ++i) {
            if (families[i].queueCount == 0) continue;
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
                break;
            }
        }

        indices.computeFamily  = PickQueueFamily(families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
        indices.transferFamily = PickQueueFamily(families, VK_QUEUE_TRANSFER_BIT,
                                                 VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

        auto canPresent = [&](uint32_t family) {
            return family < presentSupport.size() && presentSupport[family] == VK_TRUE;
        };

        //! Present: stay on the graphics family whenever it can. That keeps the
        //! swapchain EXCLUSIVE and the whole acquire -> render -> present chain on one queue
        //! TODO: Perhaps this is why imgui floating windows did not work???
        if (indices.graphicsFamily.has_value() && canPresent(*indices.graphicsFamily)) {
            indices.presentFamily = indices.graphicsFamily;
        } else {
            for (uint32_t i = 0; i < families.size(); ++i) {
                if (canPresent(i)) {
                    indices.presentFamily = i;
                    break;
                }
            }
        }

        if (forceUnified && indices.graphicsFamily.has_value()) {
            const uint32_t graphics = *indices.graphicsFamily;
            const VkQueueFlags graphicsFlags = EffectiveQueueFlags(families[graphics].queueFlags);
            indices.transferFamily = graphics;

            //! Compute is NOT implied by graphics
            if (graphicsFlags & VK_QUEUE_COMPUTE_BIT) {
                indices.computeFamily = graphics;
            } else {
                Log(Warn, "forceUnifiedQueues: graphics family {} has no compute support, keeping the separate compute family", graphics);
            }

            if (canPresent(graphics)) {
                indices.presentFamily = graphics;
            } else {
                Log(Warn, "forceUnifiedQueues: graphics family {} cannot present, keeping the separate present family", graphics);
            }
        }

        return indices;
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool forceUnified) {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

        //! A null surface means we are only asking about compute/transfer capability
        //! But tbf I don't design Shift fot this for now
        std::vector<VkBool32> presentSupport(count, VK_FALSE);
        if (surface != VK_NULL_HANDLE) {
            for (uint32_t i = 0; i < count; ++i) {
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport[i]);
            }
        }

        return SelectQueueFamilies(families, presentSupport, forceUnified);
    }

    void LogQueueFamilySelection(VkPhysicalDevice device, const QueueFamilyIndices& indices) {
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

        auto describe = [&](const char* role, const std::optional<uint32_t>& family) {
            if (!family.has_value()) {
                Log(Trace, "  {:<9} family: <none>", role);
                return;
            }
            const auto& props = families[*family];
            Log(Trace, "  {:<9} family {} (flags 0x{:X}, {} queue(s))", role, *family, props.queueFlags, props.queueCount);
        };

        Log(Trace, "Resolved queue families ({} available):", count);
        describe("graphics", indices.graphicsFamily);
        describe("present", indices.presentFamily);
        describe("compute", indices.computeFamily);
        describe("transfer", indices.transferFamily);

        //! Whole-mip copies are legal under any granularity, but partial-region copies on this
        //! family would have to be aligned to it
        if (indices.transferFamily.has_value()) {
            const auto& granularity = families[*indices.transferFamily].minImageTransferGranularity;
            const bool isUnitGranularity = granularity.width == 1 && granularity.height == 1 && granularity.depth == 1;
            if (!isUnitGranularity) {
                Log(Warn, "Transfer family {} has minImageTransferGranularity ({}, {}, {}): partial image copies on it must be aligned to that",
                    *indices.transferFamily, granularity.width, granularity.height, granularity.depth);
            }
        }
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
        if (!FindQueueFamilies(device, surface, required.VK_forceUnifiedQueues).isComplete()) return 0;

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

//! Agnostic validation-stats surface: declared in Common/Capabilities.hpp, defined by the
//! active backend (exactly one backend per binary
namespace Shift {
    ValidationStats GetValidationStats() {
        ValidationStats stats;
        stats.errorCount = VK::Util::s_validationErrors.load(std::memory_order_relaxed);
        stats.warningCount = VK::Util::s_validationWarnings.load(std::memory_order_relaxed);
        stats.infoCount = VK::Util::s_validationInfos.load(std::memory_order_relaxed);

        std::lock_guard<std::mutex> guard(VK::Util::s_validationRingMutex);
        const uint64_t total = VK::Util::s_validationRingNext;
        const uint64_t depth = std::min<uint64_t>(total, VK::Util::VALIDATION_MESSAGE_RING_DEPTH);
        stats.lastMessages.reserve(depth);
        for (uint64_t i = total - depth; i < total; ++i) {
            stats.lastMessages.push_back(VK::Util::s_validationRing[i % VK::Util::VALIDATION_MESSAGE_RING_DEPTH]);
        }
        return stats;
    }

    void ResetValidationStats() {
        VK::Util::s_validationErrors.store(0, std::memory_order_relaxed);
        VK::Util::s_validationWarnings.store(0, std::memory_order_relaxed);
        VK::Util::s_validationInfos.store(0, std::memory_order_relaxed);

        std::lock_guard<std::mutex> guard(VK::Util::s_validationRingMutex);
        for (auto& message : VK::Util::s_validationRing) {
            message.clear();
        }
        VK::Util::s_validationRingNext = 0;
    }
} // Shift