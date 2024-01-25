#ifndef SHIFT_UTILVULKAN_HPP
#define SHIFT_UTILVULKAN_HPP

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <vector>
#include <string>

namespace sft {
    namespace gutil {
        const std::vector<const char*> VALIDATION_LAYERS = {
                "VK_LAYER_KHRONOS_validation"
        };

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
        void fillDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

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
                    spdlog::debug("Validation layer: " + std::string{pCallbackData->pMessage});
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                    spdlog::info("Validation layer: " + std::string{pCallbackData->pMessage});
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                    spdlog::warn("Validation layer: " + std::string{pCallbackData->pMessage});
                    break;
                case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                    spdlog::error("Validation layer: " + std::string{pCallbackData->pMessage});
                    break;
            }

            return VK_FALSE;
        }

        //! Get the extensions required for Vulkan Instance
        std::vector<const char*> getRequiredExtensions();

        //! Checks whether required GLFW extension is present in Vulkan and prints the info
        //! For now is stupid and expensive but is supposed to run at boot either way
        bool checkForGLFWExtensionPresense(
                const char** glfwExtensions,
                uint32_t glfwExtensionCount,
                const std::vector<VkExtensionProperties>& vkExtensions);

        //! DEbug log all available extensions
        void printAvailibleVkExtensions(const std::vector<VkExtensionProperties>& vkExtensions);
    } // gutil
} //sft

#endif //SHIFT_UTILVULKAN_HPP
