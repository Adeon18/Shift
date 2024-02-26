#include "spdlog/spdlog.h"

#include "Config/EngineConfig.hpp"

#include "Device.hpp"

namespace sft {
    namespace gfx {
        Device::Device(const Instance &inst, VkSurfaceKHR surface) {
            PickPhysicalDevice(inst.Get(), surface);
            CreateLogicalDevice(surface);
            CreateAllocator(inst.Get());
        }

        Device::~Device() {
            vmaDestroyAllocator(m_allocator);
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

            const VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature {
                    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
                    .dynamicRendering = VK_TRUE
            };

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.pNext = &dynamicRenderingFeature;
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
            if (vkCreateImageView(m_device, &info, nullptr, &imageView) != VK_SUCCESS) {
                spdlog::error("Failed to create VkImageView!");
                return VK_NULL_HANDLE;
            }

            return imageView;
        }

        VkFence Device::CreateFence(const VkFenceCreateInfo& info) const {
            VkFence fence;
            if (vkCreateFence(m_device, &info, nullptr, &fence) != VK_SUCCESS) {
                spdlog::error("Failed to create VkFence!");
                return VK_NULL_HANDLE;
            }
            return fence;
        }

        void Device::DestroyFence(VkFence fence) const {
            vkDestroyFence(m_device, fence, nullptr);
        }

        VkSemaphore Device::CreateSemaphore(const VkSemaphoreCreateInfo &info) const {
            VkSemaphore semaphore;
            if (vkCreateSemaphore(m_device, &info, nullptr, &semaphore) != VK_SUCCESS) {
                spdlog::error("Failed to create VkSemaphore!");
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
            if (vkCreateCommandPool(m_device, &info, nullptr, &vkPool) != VK_SUCCESS) {
                spdlog::error("Failed to create VkCommandPool!");
                return VK_NULL_HANDLE;
            }

            return vkPool;
        }

        void Device::DestroyCommandPool(VkCommandPool pool) const {
            vkDestroyCommandPool(m_device, pool, nullptr);
        }

        VkRenderPass Device::CreateRenderPass(const VkRenderPassCreateInfo &info) const {
            VkRenderPass pass;
            if (vkCreateRenderPass(m_device, &info, nullptr, &pass) != VK_SUCCESS) {
                spdlog::error("Failed to create VkRenderPass!");
                return VK_NULL_HANDLE;
            }

            return pass;
        }

        void Device::DestroyRenderPass(VkRenderPass pass) const {
            vkDestroyRenderPass(m_device, pass, nullptr);
        }

        VkShaderModule Device::CreateShaderModule(const VkShaderModuleCreateInfo& info) const {
            VkShaderModule shaderModule;
            if (vkCreateShaderModule(m_device, &info, nullptr, &shaderModule) != VK_SUCCESS) {
                spdlog::error("Failed to create VkShaderModule!");
                return VK_NULL_HANDLE;
            }

            return shaderModule;
        }

        void Device::DestroyShaderModule(VkShaderModule module) const {
            vkDestroyShaderModule(m_device, module, nullptr);
        }

        VkPipeline Device::CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo &info) const {
            VkPipeline pipeline;
            if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS) {
                spdlog::error("Failed to create VkPipeline!");
                return VK_NULL_HANDLE;
            }
            return pipeline;
        }

        void Device::DestroyPipeline(VkPipeline pipeline) const {
            vkDestroyPipeline(m_device, pipeline, nullptr);
        }

        VkPipelineLayout Device::CreatePipelineLayout(const VkPipelineLayoutCreateInfo& info) const {
            VkPipelineLayout pipelineLayout;
            if (vkCreatePipelineLayout(m_device, &info, nullptr, &pipelineLayout) != VK_SUCCESS) {
                spdlog::error("Failed to create VkPipelineLayout!");
                return VK_NULL_HANDLE;
            }
            return pipelineLayout;
        }
        void Device::DestroyPipelineLayout(VkPipelineLayout layout) const {
            vkDestroyPipelineLayout(m_device, layout, nullptr);
        }

        VkDescriptorSetLayout Device::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo &info) const {
            VkDescriptorSetLayout layout;
            if (vkCreateDescriptorSetLayout(m_device, &info, nullptr, &layout) != VK_SUCCESS) {
                spdlog::error("Failed to create VkDescriptorSetLayout!");
                return VK_NULL_HANDLE;
            }

            return layout;
        }

        void Device::DestroyDescriptorSetLayout(VkDescriptorSetLayout layout) const {
            vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
        }

        VkDescriptorPool Device::CreateDescriptorPool(const VkDescriptorPoolCreateInfo &info) const {
            VkDescriptorPool pool;
            if (vkCreateDescriptorPool(m_device, &info, nullptr, &pool) != VK_SUCCESS) {
                spdlog::error("Failed to create VkDescriptorPool!");
                return VK_NULL_HANDLE;
            }
            return pool;
        }

        void Device::DestroyDescriptorPool(VkDescriptorPool pool) const {
            vkDestroyDescriptorPool(m_device, pool, nullptr);
        }

        VkDescriptorSet Device::AllocateDescriptorSet(const VkDescriptorSetAllocateInfo &info) const {
            VkDescriptorSet dset;
            if (vkAllocateDescriptorSets(m_device, &info, &dset) != VK_SUCCESS) {
                spdlog::error("Failed to allocate VkDescriptorSet!");
                return VK_NULL_HANDLE;
            }
            return dset;
        }

        void Device::CreateAllocator(VkInstance instance) {
            VmaAllocatorCreateInfo allocatorCreateInfo{};
            allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
            allocatorCreateInfo.vulkanApiVersion = cfg::VULKAN_VERSION;
            allocatorCreateInfo.physicalDevice = m_physicalDevice;
            allocatorCreateInfo.device = m_device;
            allocatorCreateInfo.instance = instance;

            vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
        }

        void Device::DestroyImageView(VkImageView view) const {
            vkDestroyImageView(m_device, view, nullptr);
        }

        VkSampler Device::CreateImageSampler(const VkSamplerCreateInfo &info) const {
            VkSampler sampler;
            if (vkCreateSampler(m_device, &info, nullptr, &sampler) != VK_SUCCESS) {
                spdlog::error("Failed to create VkSampler!");
                return VK_NULL_HANDLE;
            }
            return sampler;
        }

        void Device::DestroyImageSampler(VkSampler sampler) const {
            vkDestroySampler(m_device, sampler, nullptr);
        }

        VkFormat Device::FindSupportedDepthFormat() const {
            return gutil::FindSupportedFormat(
                    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    m_physicalDevice
            );
        }

        VkFramebuffer Device::CreateFrameBuffer(const VkFramebufferCreateInfo& info) const {
            VkFramebuffer buf;
            if (vkCreateFramebuffer(m_device, &info, nullptr, &buf) != VK_SUCCESS) {
                spdlog::error("Failed to create VkFramebuffer!");
                return VK_NULL_HANDLE;
            }
            return buf;
        }

        void Device::DestroyFrameBuffer(VkFramebuffer buf) const {
            vkDestroyFramebuffer(m_device, buf, nullptr);
        }
    } // gfx
} // sft
