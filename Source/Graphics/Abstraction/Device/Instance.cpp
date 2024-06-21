#include "Instance.hpp"

#include "Utility/Vulkan/UtilVulkan.hpp"
#include "Config/EngineConfig.hpp"

namespace shift {
    namespace gfx {
        Instance::Instance(std::string appName, uint32_t appVersion, std::string engName, uint32_t engVersion)
        {
            if (SHIFT_VALIDATION && !gutil::CheckValidationLayerSupport()) {
                throw VulkanCreateResourceException("Validation layers requested, but not available!");
            }

            // This struct is TECHNICALLY optional, just tells general info to driver
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = appName.c_str();
            appInfo.applicationVersion = appVersion;
            appInfo.pEngineName = engName.c_str();
            appInfo.engineVersion = engVersion;
            appInfo.apiVersion = cfg::VULKAN_VERSION;

            // This struct is NOT optional and tell about extensions and validation layers to use
            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            // Handle extensions
            auto extensions = gutil::GetRequiredExtensions();

            // This here specifies extension data for vulkan
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            // Set validation layers
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};   // Use this additional debug ext for instance creation
#if SHIFT_VALIDATION
                createInfo.enabledLayerCount = static_cast<uint32_t>(gutil::VALIDATION_LAYERS.size());
                createInfo.ppEnabledLayerNames = gutil::VALIDATION_LAYERS.data();

                gutil::FillDebugMessengerCreateInfo(debugCreateInfo);
                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
                createInfo.enabledLayerCount = 0; // Validation layers
                createInfo.pNext = nullptr;
#endif

            VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
            if (result != VK_SUCCESS) {
                throw VulkanCreateResourceException("Failed to create instance!");
            }

            PollDynamicRenderingFunctions();

#if SHIFT_VALIDATION
            SetupDebugMessenger();
#endif
        }

        void Instance::SetupDebugMessenger()  {

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            gutil::FillDebugMessengerCreateInfo(createInfo);

            if (gutil::CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
                throw VulkanCreateResourceException("Failed to set up debug messenger!");
            }
        }

        void Instance::PollDynamicRenderingFunctions() {
            vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR) vkGetInstanceProcAddr(m_instance, "vkCmdBeginRenderingKHR");
            vkCmdEndRenderingKHR   = (PFN_vkCmdEndRenderingKHR) vkGetInstanceProcAddr(m_instance, "vkCmdEndRenderingKHR");
            if (!vkCmdBeginRenderingKHR || !vkCmdEndRenderingKHR)
            {
                spdlog::error("Failed polling vkCmdBeginRenderingKHR and vkCmdEndRenderingKHR functions!");
            }
            spdlog::info("Polled vkCmdBeginRenderingKHR and vkCmdEndRenderingKHR functions!");
        }

        Instance::~Instance() {
#if SHIFT_VALIDATION
                gutil::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
#endif

            vkDestroyInstance(m_instance, nullptr); // Other resources should both be cleaned and destroyed
        }
    } // gfx
} // shift