#include "UtilVulkan.hpp"

namespace sft {
    namespace gutil {
        bool CheckValidationLayerSupport() {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            spdlog::debug("Validation layer support:");

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
                spdlog::debug("-> " + std::string{layerName} + ((layerFound) ? ": Availible" : ": Not Availible"));
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
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
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
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debugMessenger, pAllocator);
            }
        }

        void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;
        }

        std::vector<const char*> getRequiredExtensions() {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            // Check whether the extensions are avalible in vulkan
            uint32_t vkExtensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
            std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());
            if (!checkForGLFWExtensionPresense(glfwExtensions, glfwExtensionCount, vkExtensions)) {
                throw std::runtime_error("Needed GLFW Extensions are not supported by Vulkan!");
            }
            printAvailibleVkExtensions(vkExtensions);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if SHIFT_VALIDATION
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

            return extensions;
        }

        //! Checks whether required GLFW extension is present in Vulkan and prints the info
        //! For now is stupid and expensive but is supposed to run at boot either way
        bool checkForGLFWExtensionPresense(const char** glfwExtensions, uint32_t glfwExtensionCount, const std::vector<VkExtensionProperties>& vkExtensions) {
            spdlog::debug("GLFW extension support:");
            bool allExtSupported = true;
            for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
                bool present = false;
                for (auto& ext : vkExtensions) {
                    if (strcmp(ext.extensionName, glfwExtensions[i]) == 0) {
                        present = true;
                        break;
                    }
                }
                spdlog::debug("-> " + std::string{glfwExtensions[i]} + ((present) ? ": Supported" : ": Not Supported"));
                if (!present) { allExtSupported = false; }
            }

            return allExtSupported;
        }

        void printAvailibleVkExtensions(const std::vector<VkExtensionProperties>& vkExtensions) {
            spdlog::debug("Available Vk extensions:");
            for (const auto& extension : vkExtensions) {
                spdlog::debug("-> " + std::string{extension.extensionName});
            }
        }
    } // gfx
} // sft