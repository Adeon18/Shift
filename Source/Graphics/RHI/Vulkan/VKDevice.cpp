#include "Config/EngineConfig.hpp"

#include "VKDevice.hpp"

#include "VKMacros.hpp"

namespace Shift::VK {
    bool Device::Init(const Instance &inst, VkSurfaceKHR surface, const VkPhysicalDeviceFeatures& deviceFeatures) {
        if (!PickPhysicalDevice(inst.Get(), surface)) return false;
        if (!CreateLogicalDevice(deviceFeatures, surface))  return false;
        if (!CreateAllocator(inst.Get()))  return false;
    }

    void Device::Destroy() {
        vmaDestroyAllocator(m_allocator);
        vkDestroyDevice(m_device, nullptr);
    }

    bool Device::CreateLogicalDevice(const VkPhysicalDeviceFeatures& deviceFeatures, VkSurfaceKHR surface) {
        m_queueFamilyIndices = Util::FindQueueFamilies(m_physicalDevice, surface);

        // You don't really need more than 1 queue for each queue family
        // That is because you can create all the command buffers on multiple threads and submit them at once to the main thread
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
                m_queueFamilyIndices.graphicsFamily.value(),
                m_queueFamilyIndices.presentFamily.value(),
                m_queueFamilyIndices.transferFamily.value()
        };

        Log(Trace, "Graphics family: " + std::to_string(*m_queueFamilyIndices.graphicsFamily));
        Log(Trace, "Transfer family: " + std::to_string(*m_queueFamilyIndices.transferFamily));
        Log(Trace, "Present family: " + std::to_string(*m_queueFamilyIndices.presentFamily));

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            // You must specify the priority to the queue in vulkan
            queueCreateInfo.pQueuePriorities = &Util::DEFAULT_QUEUE_PRIORITY;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // We will use the default features
        // TODO: make this congigurable through constructor
        VkPhysicalDeviceFeatures physDeviceFeatures{ deviceFeatures };

        const VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
                .dynamicRendering = VK_TRUE
        };

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &dynamicRenderingFeature;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &physDeviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(Util::DEVICE_EXTENSIONS.size());
        createInfo.ppEnabledExtensionNames = Util::DEVICE_EXTENSIONS.data();
#if SHIFT_VALIDATION
            createInfo.enabledLayerCount = static_cast<uint32_t>(Util::VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = Util::VALIDATION_LAYERS.data();
#else
            createInfo.enabledLayerCount = 0;
#endif
        if (VkCheck(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device))) {
            LogVerbose(Critical, "Failed to create logical device!");
            return false;
        }

        // Pull the queue handles
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);

        return true;
    }

    bool Device::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            LogVerbose(Critical, "Failed to find GPUs with Vulkan support!");
            return false;
        }

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        Util::PrintAvailablePhysicalDevices(physicalDevices);

        // Use an ordered map to automatically sort candidates by increasing score
        std::multimap<int, VkPhysicalDevice> candidates;

        for (const auto& device : physicalDevices) {
            int score = Util::RateDeviceSuitability(device, surface);
            candidates.insert(std::make_pair(score, device));
        }

        // Check if the best candidate is suitable at all
        if (candidates.rbegin()->first > 0) {
            m_physicalDevice = candidates.rbegin()->second;
            Util::PrintDeviceName(m_physicalDevice, "Chosen device: ");
        }
        else {
            LogVerbose(Critical, "Failed to find a suitable GPU!");
            return false;
        }

        vkGetPhysicalDeviceProperties(m_physicalDevice, &m_deviceProperties);

        if (m_deviceProperties.limits.timestampPeriod == 0) {
            LogVerbose(Critical, "GPU does not support timestemp queries!");
            return false;
        }

        if (!m_deviceProperties.limits.timestampComputeAndGraphics) {
            LogVerbose(Critical, "GPU does not support timestamp queries for all queue families!");
            return false;
        }

        return true;
    }

    VkImageView Device::CreateImageView(const VkImageViewCreateInfo& info) const {
        VkImageView imageView;
        if (VkCheck(vkCreateImageView(m_device, &info, nullptr, &imageView))) {
            Log(Error, "Failed to create VkImageView!");
            return VK_NULL_HANDLE;
        }

        return imageView;
    }

    VkFence Device::CreateFence(const VkFenceCreateInfo& info) const {
        VkFence fence;
        if (VkCheck(vkCreateFence(m_device, &info, nullptr, &fence))) {
            Log(Error, "Failed to create VkFence!");
            return VK_NULL_HANDLE;
        }
        return fence;
    }

    void Device::DestroyFence(VkFence fence) const {
        vkDestroyFence(m_device, fence, nullptr);
    }

    VkSemaphore Device::CreateSemaphore(const VkSemaphoreCreateInfo &info) const {
        VkSemaphore semaphore;
        if ( VkCheck(vkCreateSemaphore(m_device, &info, nullptr, &semaphore)) ) {
            Log(Error, "Failed to create VkSemaphore!");
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
        if ( VkCheck(vkCreateCommandPool(m_device, &info, nullptr, &vkPool)) ) {
            Log(Error, "Failed to create VkCommandPool!");
            return VK_NULL_HANDLE;
        }

        return vkPool;
    }

    void Device::DestroyCommandPool(VkCommandPool pool) const {
        vkDestroyCommandPool(m_device, pool, nullptr);
    }

    VkRenderPass Device::CreateRenderPass(const VkRenderPassCreateInfo &info) const {
        VkRenderPass pass;
        if (VkCheck(vkCreateRenderPass(m_device, &info, nullptr, &pass)) ) {
            Log(Error, "Failed to create VkRenderPass!");
            return VK_NULL_HANDLE;
        }

        return pass;
    }

    void Device::DestroyRenderPass(VkRenderPass pass) const {
        vkDestroyRenderPass(m_device, pass, nullptr);
    }

    VkShaderModule Device::CreateShaderModule(const VkShaderModuleCreateInfo& info) const {
        VkShaderModule shaderModule;
        if ( VkCheck(vkCreateShaderModule(m_device, &info, nullptr, &shaderModule)) ) {
            Log(Error, "Failed to create VkShaderModule!");
            return VK_NULL_HANDLE;
        }

        return shaderModule;
    }

    void Device::DestroyShaderModule(VkShaderModule module) const {
        vkDestroyShaderModule(m_device, module, nullptr);
    }

    VkPipeline Device::CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo &info) const {
        VkPipeline pipeline;
        if ( VkCheck(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline)) ) {
            Log(Error, "Failed to create VkPipeline!");
            return VK_NULL_HANDLE;
        }
        return pipeline;
    }

    void Device::DestroyPipeline(VkPipeline pipeline) const {
        vkDestroyPipeline(m_device, pipeline, nullptr);
    }

    VkPipelineLayout Device::CreatePipelineLayout(const VkPipelineLayoutCreateInfo& info) const {
        VkPipelineLayout pipelineLayout;
        if ( VkCheck(vkCreatePipelineLayout(m_device, &info, nullptr, &pipelineLayout)) ) {
            Log(Error, "Failed to create VkPipelineLayout!");
            return VK_NULL_HANDLE;
        }
        return pipelineLayout;
    }
    void Device::DestroyPipelineLayout(VkPipelineLayout layout) const {
        vkDestroyPipelineLayout(m_device, layout, nullptr);
    }

    VkDescriptorSetLayout Device::CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo &info) const {
        VkDescriptorSetLayout layout;
        if ( VkCheck(vkCreateDescriptorSetLayout(m_device, &info, nullptr, &layout)) ) {
            Log(Error, "Failed to create VkDescriptorSetLayout!");
            return VK_NULL_HANDLE;
        }

        return layout;
    }

    void Device::DestroyDescriptorSetLayout(VkDescriptorSetLayout layout) const {
        vkDestroyDescriptorSetLayout(m_device, layout, nullptr);
    }

    VkDescriptorPool Device::CreateDescriptorPool(const VkDescriptorPoolCreateInfo &info) const {
        VkDescriptorPool pool;
        if ( VkCheck(vkCreateDescriptorPool(m_device, &info, nullptr, &pool)) ) {
            Log(Error, "Failed to create VkDescriptorPool!");
            return VK_NULL_HANDLE;
        }
        return pool;
    }

    void Device::DestroyDescriptorPool(VkDescriptorPool pool) const {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }

    VkDescriptorSet Device::AllocateDescriptorSet(const VkDescriptorSetAllocateInfo &info) const {
        VkDescriptorSet dset;
        if ( VkCheck(vkAllocateDescriptorSets(m_device, &info, &dset)) ) {
            Log(Error, "Failed to allocate VkDescriptorSet!");
            return VK_NULL_HANDLE;
        }
        return dset;
    }

    bool Device::CreateAllocator(VkInstance instance) {
        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = cfg::VULKAN_VERSION;
        allocatorCreateInfo.physicalDevice = m_physicalDevice;
        allocatorCreateInfo.device = m_device;
        allocatorCreateInfo.instance = instance;

        if ( VkCheck(vmaCreateAllocator(&allocatorCreateInfo, &m_allocator)) ) {
            LogVerbose(Critical, "Failed to create the VMA Allocator");
            return false;
        }

        return true;
    }

    void Device::DestroyImageView(VkImageView view) const {
        vkDestroyImageView(m_device, view, nullptr);
    }

    VkSampler Device::CreateImageSampler(const VkSamplerCreateInfo &info) const {
        VkSampler sampler;
        if ( VkCheck(vkCreateSampler(m_device, &info, nullptr, &sampler))) {
            Log(Error, "Failed to create VkSampler!");
            return VK_NULL_HANDLE;
        }
        return sampler;
    }

    void Device::DestroyImageSampler(VkSampler sampler) const {
        vkDestroySampler(m_device, sampler, nullptr);
    }

    VkFormat Device::FindSupportedDepthFormat() const {
        return Util::FindSupportedFormat(
                {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
                m_physicalDevice
        );
    }

    VkFramebuffer Device::CreateFrameBuffer(const VkFramebufferCreateInfo& info) const {
        VkFramebuffer buf;
        if ( VkCheck(vkCreateFramebuffer(m_device, &info, nullptr, &buf)) ) {
            Log(Error, "Failed to create VkFramebuffer!");
            return VK_NULL_HANDLE;
        }
        return buf;
    }

    void Device::DestroyFrameBuffer(VkFramebuffer buf) const {
        vkDestroyFramebuffer(m_device, buf, nullptr);
    }

    VkQueryPool Device::CreateQueryPool(const VkQueryPoolCreateInfo &info) const {
        VkQueryPool pool;

        if ( VkCheck(vkCreateQueryPool(m_device, &info, nullptr, &pool)) ) {
            Log(Error, "Failed to create VkQueryPool!");
            return VK_NULL_HANDLE;
        }
        return pool;
    }

    void Device::DestroyQueryPool(VkQueryPool pool) const {
        vkDestroyQueryPool(m_device, pool, nullptr);
    }
} // Shift::VK
