#include "VKInstance.hpp"

#include "Utility/Vulkan/VKUtilCore.hpp"
#include "Config/EngineConfig.hpp"

namespace Shift::VK {
    bool Instance::Init(const std::string& appName, uint32_t appVersion, const std::string& engName, uint32_t engVersion)
    {
        if (SHIFT_VALIDATION && !Util::CheckValidationLayerSupport()) {
            LogVerbose(Error, "Validation Layers are not available!");
        }

        // This struct is TECHNICALLY optional, just tells general info to driver
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = appName.c_str();
        appInfo.applicationVersion = appVersion;
        appInfo.pEngineName = engName.c_str();
        appInfo.engineVersion = engVersion;
        appInfo.apiVersion = Conf::VULKAN_VERSION;

        // This struct is NOT optional and tell about extensions and validation layers to use
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Handle extensions
        auto extensions = Util::GetRequiredExtensions();

        // This here specifies extension data for vulkan
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Set validation layers
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};   // Use this additional debug ext for instance creation
#if SHIFT_VALIDATION
            createInfo.enabledLayerCount = static_cast<uint32_t>(Util::VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = Util::VALIDATION_LAYERS.data();

            Util::FillDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
            createInfo.enabledLayerCount = 0; // Validation layers
            createInfo.pNext = nullptr;
#endif
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if (result != VK_SUCCESS) {
            LogVerbose(Critical, "Failed to create instance!");
            return false;
        }

        if (!PollDynamicRenderingFunctions()) return false;

#if SHIFT_VALIDATION
        if (!SetupDebugMessenger()) return false;
#endif
        return true;
    }

    bool Instance::SetupDebugMessenger()  {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        Util::FillDebugMessengerCreateInfo(createInfo);

        if (Util::CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            LogVerbose(Critical, "Failed to set up a debug messenger!");
            return false;
        }

        return true;
    }

    bool Instance::PollDynamicRenderingFunctions() {
        vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR) vkGetInstanceProcAddr(m_instance, "vkCmdBeginRenderingKHR");
        vkCmdEndRenderingKHR   = (PFN_vkCmdEndRenderingKHR) vkGetInstanceProcAddr(m_instance, "vkCmdEndRenderingKHR");
        if (!vkCmdBeginRenderingKHR || !vkCmdEndRenderingKHR)
        {
            LogVerbose(Critical, "Failed polling vkCmdBeginRenderingKHR and vkCmdEndRenderingKHR functions!");
            return false;
        }
        Log(Info, "Polled vkCmdBeginRenderingKHR and vkCmdEndRenderingKHR functions!");
        return true;
    }

    void Instance::Destroy() {
#if SHIFT_VALIDATION
            Util::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif
        vkDestroyInstance(m_instance, nullptr);
    }
} // Shift::VK