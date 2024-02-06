#include "spdlog/spdlog.h"

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
            m_queueFamilyIndices = gutil::FindQueueFamilies(m_physicalDevice, surface);

            // You don't really need more than 1 queue for each queue family
            // That is because you can create all the command buffers on multiple threads and submit them at once to the main thread
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {
                    m_queueFamilyIndices.graphicsFamily.value(),
                    m_queueFamilyIndices.presentFamily.value(),
                    m_queueFamilyIndices.transferFamily.value()
            };

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
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);
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
                throw VulkanCreateResourceException("Failed to find a suitable GPU!");
            }
        }

        VkImageView Device::CreateImageView(const VkImageViewCreateInfo& info) const {
            VkImageView imageView;
            if (int res = vkCreateImageView(m_device, &info, nullptr, &imageView); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkImageView! Code: {}", res);
                return VK_NULL_HANDLE;
            }

            return imageView;
        }

        VkFence Device::CreateFence(const VkFenceCreateInfo& info) const {
            VkFence fence;
            if (int res = vkCreateFence(m_device, &info, nullptr, &fence); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkFence! Code: {}", res);
                return VK_NULL_HANDLE;
            }
            return fence;
        }

        void Device::DestroyFence(VkFence fence) const {
            vkDestroyFence(m_device, fence, nullptr);
        }

        VkSemaphore Device::CreateSemaphore(const VkSemaphoreCreateInfo &info) const {
            VkSemaphore semaphore;
            if (int res = vkCreateSemaphore(m_device, &info, nullptr, &semaphore); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkSemaphore! Code: {}", res);
                return VK_NULL_HANDLE;
            }
            return semaphore;
        }

        void Device::DestroySemaphore(VkSemaphore semaphore) const {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }

        VkCommandPool Device::CreateCommandPool(const VkCommandPoolCreateInfo &info) const {
            VkCommandPool vkPool;
            //! INFO: Command buffers are executed by submitting them on one of the device queues, like the graphics
            //! and presentation queues we retrieved.Each command pool can only allocate command buffers that are
            //! submitted on a single type of queue.We're going to record commands for drawing, which is why we've chosen the graphics queue family.
            if (int res = vkCreateCommandPool(m_device, &info, nullptr, &vkPool); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkCommandPool! Code: {}", res);
                return VK_NULL_HANDLE;
            }

            return vkPool;
        }

        void Device::DestroyCommandPool(VkCommandPool pool) const {
            vkDestroyCommandPool(m_device, pool, nullptr);
        }

        VkRenderPass Device::CreateRenderPass(const VkRenderPassCreateInfo &info) const {
            VkRenderPass pass;
            if (int res = vkCreateRenderPass(m_device, &info, nullptr, &pass); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkRenderPass! Code: {}", res);
                return VK_NULL_HANDLE;
            }

            return pass;
        }

        void Device::DestroyRenderPass(VkRenderPass pass) const {
            vkDestroyRenderPass(m_device, pass, nullptr);
        }

        VkShaderModule Device::CreateShaderModule(const VkShaderModuleCreateInfo& info) const {
            VkShaderModule shaderModule;
            if (int res = vkCreateShaderModule(m_device, &info, nullptr, &shaderModule); res != VK_SUCCESS) {
                spdlog::error("Failed to create VkShaderModule! Code: {}", res);
                return VK_NULL_HANDLE;
            }

            return shaderModule;
        }

        void Device::DestroyShaderModule(VkShaderModule module) const {
            vkDestroyShaderModule(m_device, module, nullptr);
        }

    } // gfx
} // sft
