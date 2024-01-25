#include "Device.hpp"

namespace sft {
    namespace gfx {
        Device::Device(const Instance &inst, VkSurfaceKHR surface) {
            PickPhysicalDevice(inst.Get(), surface);
            CreateLogicalDevice(surface);
        }

        Device::~Device() {
            vkDestroyDevice(m_device, nullptr);
        }

        void Device::CreateLogicalDevice(VkSurfaceKHR surface) {
            gutil::QueueFamilyIndices indices = gutil::FindQueueFamilies(m_physicalDevice, surface);

            // You don't really need more than 1 queue for each queue family
            // That is because you can create all the command buffers on multiple threads and submit them at once to the main thread
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                // You must specify the priority to the queue in vulkan
                queueCreateInfo.pQueuePriorities = &gutil::DEFAULT_QUEUE_PRIORITY;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            // We will use the default features
            // TODO: make this congigurable thought constructor
            VkPhysicalDeviceFeatures deviceFeatures{};
            // We want an anisotropic sampler
            deviceFeatures.samplerAnisotropy = VK_TRUE;

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.pEnabledFeatures = &deviceFeatures;
            createInfo.enabledExtensionCount = gutil::DEVICE_EXTENSIONS.size();
            createInfo.ppEnabledExtensionNames = gutil::DEVICE_EXTENSIONS.data();
#if SHIFT_VALIDATION
                createInfo.enabledLayerCount = static_cast<uint32_t>(sft::gutil::VALIDATION_LAYERS.size());
                createInfo.ppEnabledLayerNames = gutil::VALIDATION_LAYERS.data();
#else
                createInfo.enabledLayerCount = 0;
#endif
            if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
                throw VulkanCreateResourceException("Failed to create logical device!");
            }

            // Pull the queue handles
            vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
            vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
            vkGetDeviceQueue(m_device, indices.transferFamily.value(), 0, &m_transferQueue);
        }

        void Device::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            if (deviceCount == 0) {
                throw VulkanCreateResourceException("Failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

            gutil::PrintAvailablePhysicalDevices(physicalDevices);

            // Use an ordered map to automatically sort candidates by increasing score
            std::multimap<int, VkPhysicalDevice> candidates;

            for (const auto& device : physicalDevices) {
                int score = gutil::RateDeviceSuitability(device, surface);
                candidates.insert(std::make_pair(score, device));
            }

            // Check if the best candidate is suitable at all
            if (candidates.rbegin()->first > 0) {
                m_physicalDevice = candidates.rbegin()->second;
                gutil::PrintDeviceName(m_physicalDevice, "Chosen device: ");
            }
            else {
                throw std::runtime_error("failed to find a suitable GPU!");
            }
        }
    } // gfx
} // sft
