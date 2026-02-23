#include "VKInstance.hpp"

#include "Utility/Vulkan/VKUtilCore.hpp"
#include "Config/EngineConfig.hpp"

namespace Shift::VK {
    Instance::Instance(const std::string& appName, uint32_t appVersion, const std::string& engName, uint32_t engVersion, const RHIRequiredFeatures& required)
    {
        if (VkCheck(volkInitialize())) {
            Log(Critical, "Failed to initialize Volk!");
            return;
        }

        //! TODO: THIS IS STUPID
        if (required.VK_enableValidationLayers && !Util::CheckValidationLayerSupport()) {
            LogVerbose(Error, "Validation layers requested but not available!");
            if (required.VK_enableValidationLayers) {
                LogVerbose(Critical, "Validation layers are REQUIRED but missing. Aborting.");
                return;
            }
        }

        // Application info (optional but recommended)
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName.c_str();
        appInfo.applicationVersion = appVersion;
        appInfo.pEngineName = engName.c_str();
        appInfo.engineVersion = engVersion;
        appInfo.apiVersion = Conf::VULKAN_VERSION; // Vulkan 1.3 baseline

        // Get required instance extensions (GLFW + debug if validation enabled)
        auto extensions = Util::GetRequiredInstanceExtensions(required);

        // Instance create info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Debug messenger for instance creation/destruction messages
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    #if SHIFT_VALIDATION
        if (required.VK_enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(Util::VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = Util::VALIDATION_LAYERS.data();

            // Chain debug messenger for instance-level validation
            Util::FillDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
    #else
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    #endif

        // Create the Vulkan instance
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if (result != VK_SUCCESS) {
            LogVerbose(Critical, "Failed to create Vulkan instance!");
            return;
        }

        volkLoadInstance(m_instance);

        Log(
            Info,
            "Running Vulkan {}",
            std::to_string(VK_VERSION_MAJOR(Conf::VULKAN_VERSION)) + std::string(".") + std::to_string(VK_VERSION_MINOR(Conf::VULKAN_VERSION))
        );


    #if SHIFT_VALIDATION
        // Setup debug messenger for runtime validation messages
        if (required.VK_enableValidationLayers) {
            if (!SetupDebugMessenger()) {
                return;
            }
        }
    #endif

        m_valid = VkNullCheck(m_instance);
    }

    bool Instance::SetupDebugMessenger()  {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        Util::FillDebugMessengerCreateInfo(createInfo);

        if (VkCheck(vkCreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger))) {
            LogVerbose(Critical, "Failed to set up a debug messenger!");
            return false;
        }

        return true;
    }


    VkResult Instance::WaitForPresent(VkDevice dev, VkSwapchainKHR swapchain, uint64_t waitId, uint64_t timeout) const {
        return m_fpWaitForPresentKHR(dev, swapchain, waitId, timeout);
    }

    Instance::~Instance() {
#if SHIFT_VALIDATION
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
        vkDestroyInstance(m_instance, nullptr);
    }
} // Shift::VK