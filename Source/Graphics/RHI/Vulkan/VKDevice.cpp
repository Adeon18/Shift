#include "Config/EngineConfig.hpp"

#include "../Common/Capabilities.hpp"

#include "VKDevice.hpp"

#include "VKMacros.hpp"

#include "Utility/Assertions.hpp"

namespace Shift::VK {

    Device::Device(const Instance &inst, VkSurfaceKHR surface, const RHIRequiredFeatures& deviceFeatures): m_required{deviceFeatures} {
        CheckCriticalEmptyReturn(PickPhysicalDevice(inst.Get(), surface, m_required), "Failed to pick the physical device!");
        CheckCriticalEmptyReturn(FillCapabilitiesAndBuildFeatureChain(m_required, surface), "Failed to satisfly required device features!");
        CheckCriticalEmptyReturn(CreateLogicalDevice(surface), "Failed to create logical device!");
        CheckCriticalEmptyReturn(CreateAllocator(inst.Get()), "Failed to create the allocator!");

        m_valid = true;
    }

    Device::~Device() {
        vmaDestroyAllocator(m_allocator);
        vkDestroyDevice(m_device, nullptr);
    }

    bool Device::FillCapabilitiesAndBuildFeatureChain(const RHIRequiredFeatures &required, VkSurfaceKHR surface) {
        // copy requested flags
        auto &req = required;
        auto &f = m_caps.features;

        f.robustBufferAccess.requested   = req.robustBufferAccess;
        f.fullDrawIndexUint32.requested  = req.fullDrawIndexUint32;
        f.imageCubeArray.requested       = req.imageCubeArray;
        f.independentBlend.requested     = req.independentBlend;
        f.geometryShader.requested       = req.geometryShader;
        f.tessellationShader.requested   = req.tessellationShader;
        f.multiDrawIndirect.requested    = req.multiDrawIndirect;
        f.drawIndirectCount.requested    = req.drawIndirectCount;
        f.samplerAnisotropy.requested    = req.samplerAnisotropy;
        f.depthClamp.requested           = req.depthClamp;
        f.fillModeNonSolid.requested     = req.fillModeNonSolid;
        f.pipelineStatisticsQuery.requested = req.pipelineStatisticsQuery;
        f.textureCompressionBC.requested = req.textureCompressionBC;
        f.textureCompressionASTC.requested = req.textureCompressionASTC;
        f.textureCompressionETC2.requested = req.textureCompressionETC2;
        f.dualSrcBlend.requested         = req.dualSrcBlend;
        f.computeShader.requested        = req.computeShader;
        f.VK_timelineSemaphores.requested   = req.VK_timelineSemaphore;
        f.VK_descriptorIndexing.requested   = req.VK_descriptorIndexing;
        f.VK_dynamicRendering.requested     = req.VK_dynamicRendering;
        f.VK_synchronization2.requested     = req.VK_synchronization2;
        f.VK_bufferDeviceAddress.requested  = req.VK_bufferDeviceAddress;
        f.VK_scalarBlockLayout.requested    = req.VK_scalarBlockLayout;
        f.VK_hostQueryReset.requested       = req.VK_hostQueryReset;
        f.VK_presentWait.requested          = req.VK_presentWait;
        f.VK_maintenance1.requested          = req.VK_maintenance1;

        std::memset(&m_enabledFeatures, 0, sizeof(m_enabledFeatures));

        m_enabledFeatures.features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        m_enabledFeatures.vk11.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        m_enabledFeatures.vk12.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        m_enabledFeatures.vk13.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        m_enabledFeatures.presentWait.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
        m_enabledFeatures.presentId.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
        m_enabledFeatures.maintenance1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;

        m_enabledFeatures.features2.pNext = &m_enabledFeatures.vk11;
        m_enabledFeatures.vk11.pNext = &m_enabledFeatures.vk12;
        m_enabledFeatures.vk12.pNext = &m_enabledFeatures.vk13;
        m_enabledFeatures.vk13.pNext = &m_enabledFeatures.presentWait;
        m_enabledFeatures.presentWait.pNext = &m_enabledFeatures.presentId;
        m_enabledFeatures.presentId.pNext = &m_enabledFeatures.maintenance1;

        // query features into persistent memory
        vkGetPhysicalDeviceFeatures2(m_physicalDevice, &m_enabledFeatures.features2);

        // queue availability
        auto qIndices = Util::FindQueueFamilies(m_physicalDevice, VK_NULL_HANDLE);
        bool hasComputeQueue = qIndices.computeFamily.has_value();

        // Fill supported fields
        f.robustBufferAccess.supported   = m_enabledFeatures.features2.features.robustBufferAccess ? true : false;
        f.fullDrawIndexUint32.supported  = m_enabledFeatures.features2.features.fullDrawIndexUint32 ? true : false;
        f.imageCubeArray.supported       = m_enabledFeatures.features2.features.imageCubeArray ? true : false;
        f.independentBlend.supported     = m_enabledFeatures.features2.features.independentBlend ? true : false;
        f.geometryShader.supported       = m_enabledFeatures.features2.features.geometryShader ? true : false;
        f.tessellationShader.supported   = m_enabledFeatures.features2.features.tessellationShader ? true : false;
        f.multiDrawIndirect.supported    = m_enabledFeatures.features2.features.multiDrawIndirect ? true : false;
        f.samplerAnisotropy.supported    = m_enabledFeatures.features2.features.samplerAnisotropy ? true : false;
        f.depthClamp.supported           = m_enabledFeatures.features2.features.depthClamp ? true : false;
        f.fillModeNonSolid.supported     = m_enabledFeatures.features2.features.fillModeNonSolid ? true : false;
        f.pipelineStatisticsQuery.supported = m_enabledFeatures.features2.features.pipelineStatisticsQuery ? true : false;
        f.textureCompressionBC.supported = m_enabledFeatures.features2.features.textureCompressionBC ? true : false;
        f.textureCompressionASTC.supported = m_enabledFeatures.features2.features.textureCompressionASTC_LDR ? true : false;
        f.textureCompressionETC2.supported = m_enabledFeatures.features2.features.textureCompressionETC2 ? true : false;
        f.computeShader.supported        = hasComputeQueue;
        f.dualSrcBlend.supported         = m_enabledFeatures.features2.features.dualSrcBlend ? true : false;

        // 1.2 / 1.3 promoted features
        f.VK_timelineSemaphores.supported   = m_enabledFeatures.vk12.timelineSemaphore ? true : false;
        //! Bindless requires runtime arrays + partially-bound + variable-count + sampled-image update-after-bind +
        //! update-unused-while-pending + non-uniform sampled-image indexing
        bool supportsBindless =
            m_enabledFeatures.vk12.descriptorIndexing &&
            m_enabledFeatures.vk12.runtimeDescriptorArray &&
            m_enabledFeatures.vk12.descriptorBindingPartiallyBound &&
            m_enabledFeatures.vk12.descriptorBindingVariableDescriptorCount &&
            m_enabledFeatures.vk12.descriptorBindingSampledImageUpdateAfterBind &&
            m_enabledFeatures.vk12.descriptorBindingUpdateUnusedWhilePending &&
            m_enabledFeatures.vk12.shaderSampledImageArrayNonUniformIndexing;
        f.VK_descriptorIndexing.supported = supportsBindless;
        f.VK_hostQueryReset.supported       = m_enabledFeatures.vk12.hostQueryReset ? true : false;
        f.VK_dynamicRendering.supported     = m_enabledFeatures.vk13.dynamicRendering ? true : false;
        f.VK_synchronization2.supported     = m_enabledFeatures.vk13.synchronization2 ? true : false;
        f.VK_bufferDeviceAddress.supported  = m_enabledFeatures.vk12.bufferDeviceAddress ? true : false;
        f.VK_scalarBlockLayout.supported    = m_enabledFeatures.vk12.scalarBlockLayout ? true : false;

        // Extensions
        f.VK_presentWait.supported          = m_enabledFeatures.presentWait.presentWait ? true : false;
        f.VK_maintenance1.supported          = m_enabledFeatures.maintenance1.swapchainMaintenance1 ? true : false;

        // Fill vkExtensions convenience flags
        m_caps.vkExtensions.timelineSemaphore = f.VK_timelineSemaphores.supported;
        m_caps.vkExtensions.descriptorIndexing = f.VK_descriptorIndexing.supported;
        m_caps.vkExtensions.dynamicRendering = f.VK_dynamicRendering.supported;
        m_caps.vkExtensions.synchronization2 = f.VK_synchronization2.supported;
        m_caps.vkExtensions.bufferDeviceAddress = f.VK_bufferDeviceAddress.supported;
        m_caps.vkExtensions.scalarBlockLayout = f.VK_scalarBlockLayout.supported;
        m_caps.vkExtensions.hostQueryReset = f.VK_hostQueryReset.supported;
        m_caps.vkExtensions.samplerAnisotropy = f.samplerAnisotropy.supported;
        m_caps.vkExtensions.multiDrawIndirect = f.multiDrawIndirect.supported;
        m_caps.vkExtensions.drawIndirectCount = f.drawIndirectCount.supported;
        m_caps.vkExtensions.presentWait = f.VK_presentWait.supported;
        m_caps.vkExtensions.maintenance1 = f.VK_maintenance1.supported;

        // Fill limits & version
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
        m_caps.limits.maxTextureDimension1D = props.limits.maxImageDimension1D;
        m_caps.limits.maxTextureDimension2D = props.limits.maxImageDimension2D;
        m_caps.limits.maxTextureDimension3D = props.limits.maxImageDimension3D;
        m_caps.limits.maxTextureArrayLayers = props.limits.maxImageArrayLayers;
        m_caps.limits.maxVertexAttributes = props.limits.maxVertexInputAttributes;
        m_caps.limits.maxColorAttachments = props.limits.maxColorAttachments;
        m_caps.limits.maxComputeWorkGroupCount[0] = props.limits.maxComputeWorkGroupCount[0];
        m_caps.limits.maxComputeWorkGroupCount[1] = props.limits.maxComputeWorkGroupCount[1];
        m_caps.limits.maxComputeWorkGroupCount[2] = props.limits.maxComputeWorkGroupCount[2];
        m_caps.limits.maxComputeWorkGroupSize[0] = props.limits.maxComputeWorkGroupSize[0];
        m_caps.limits.maxComputeWorkGroupSize[1] = props.limits.maxComputeWorkGroupSize[1];
        m_caps.limits.maxComputeWorkGroupSize[2] = props.limits.maxComputeWorkGroupSize[2];
        m_caps.limits.maxComputeSharedMemorySize = props.limits.maxComputeSharedMemorySize;

        m_caps.version.apiVersionMajor = VK_VERSION_MAJOR(props.apiVersion);
        m_caps.version.apiVersionMinor = VK_VERSION_MINOR(props.apiVersion);
        m_caps.version.driverVersion = props.driverVersion;
        m_caps.version.deviceName = !std::string{props.deviceName}.empty() ? props.deviceName : std::string("Unknown");

        //! Hard-fail if requested & unsupported
        auto require_or_fail = [&](const char* name, const RHICommonFeature& feat)->bool {
            if (feat.requested && !feat.supported) {
                LogVerbose(Critical, std::string("Requested feature not supported by physical device: ") + name);
                return false;
            }
            return true;
        };

        if (!require_or_fail("geometryShader", f.geometryShader)) return false;
        if (!require_or_fail("tessellationShader", f.tessellationShader)) return false;
        if (!require_or_fail("samplerAnisotropy", f.samplerAnisotropy)) return false;
        if (!require_or_fail("computeShader", f.computeShader)) return false;
        if (!require_or_fail("timelineSemaphores", f.VK_timelineSemaphores)) return false;
        if (!require_or_fail("descriptorIndexing", f.VK_descriptorIndexing)) return false;
        if (!require_or_fail("dynamicRendering", f.VK_dynamicRendering)) return false;
        if (!require_or_fail("synchronization2", f.VK_synchronization2)) return false;
        if (!require_or_fail("bufferDeviceAddress", f.VK_bufferDeviceAddress)) return false;
        if (!require_or_fail("scalarBlockLayout", f.VK_scalarBlockLayout)) return false;
        if (!require_or_fail("hostQueryReset", f.VK_hostQueryReset)) return false;
        if (!require_or_fail("presentWait", f.VK_presentWait)) return false;
        if (!require_or_fail("maintenance1", f.VK_maintenance1)) return false;

        // Build enabled feature structs for vkCreateDevice (enable only requested ones)
        if (f.robustBufferAccess.requested) m_enabledFeatures.core.robustBufferAccess = VK_TRUE;
        if (f.fullDrawIndexUint32.requested) m_enabledFeatures.core.fullDrawIndexUint32 = VK_TRUE;
        if (f.imageCubeArray.requested) m_enabledFeatures.core.imageCubeArray = VK_TRUE;
        if (f.independentBlend.requested) m_enabledFeatures.core.independentBlend = VK_TRUE;
        if (f.geometryShader.requested) m_enabledFeatures.core.geometryShader = VK_TRUE;
        if (f.tessellationShader.requested) m_enabledFeatures.core.tessellationShader = VK_TRUE;
        if (f.multiDrawIndirect.requested) m_enabledFeatures.core.multiDrawIndirect = VK_TRUE;
        if (f.samplerAnisotropy.requested) m_enabledFeatures.core.samplerAnisotropy = VK_TRUE;
        if (f.depthClamp.requested) m_enabledFeatures.core.depthClamp = VK_TRUE;
        if (f.fillModeNonSolid.requested) m_enabledFeatures.core.fillModeNonSolid = VK_TRUE;
        if (f.pipelineStatisticsQuery.requested) m_enabledFeatures.core.pipelineStatisticsQuery = VK_TRUE;
        if (f.textureCompressionBC.requested) m_enabledFeatures.core.textureCompressionBC = VK_TRUE;
        if (f.textureCompressionASTC.requested) m_enabledFeatures.core.textureCompressionASTC_LDR = VK_TRUE;
        if (f.textureCompressionETC2.requested) m_enabledFeatures.core.textureCompressionETC2 = VK_TRUE;
        if (f.dualSrcBlend.requested) m_enabledFeatures.core.dualSrcBlend = VK_TRUE;

        //! Make sure we only enabled what we requested
        m_enabledFeatures.vk11 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        m_enabledFeatures.vk12 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        m_enabledFeatures.vk13 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };

        // set requested -> enable in vk12/vk13
        m_enabledFeatures.vk12.timelineSemaphore  = f.VK_timelineSemaphores.requested ? VK_TRUE : VK_FALSE;
        if (f.VK_descriptorIndexing.requested) {
            m_enabledFeatures.vk12.descriptorIndexing = VK_TRUE;
            m_enabledFeatures.vk12.runtimeDescriptorArray = VK_TRUE;
            m_enabledFeatures.vk12.descriptorBindingPartiallyBound = VK_TRUE;
            m_enabledFeatures.vk12.descriptorBindingVariableDescriptorCount = VK_TRUE;
            //! Required by the bindless texture layout
            m_enabledFeatures.vk12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
            m_enabledFeatures.vk12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
            m_enabledFeatures.vk12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        }
        m_enabledFeatures.vk12.hostQueryReset     = f.VK_hostQueryReset.requested ? VK_TRUE : VK_FALSE;
        m_enabledFeatures.vk12.bufferDeviceAddress = f.VK_bufferDeviceAddress.requested ? VK_TRUE : VK_FALSE;
        m_enabledFeatures.vk12.scalarBlockLayout  = f.VK_scalarBlockLayout.requested ? VK_TRUE : VK_FALSE;
        m_enabledFeatures.vk13.dynamicRendering   = f.VK_dynamicRendering.requested ? VK_TRUE : VK_FALSE;
        m_enabledFeatures.vk13.synchronization2   = f.VK_synchronization2.requested ? VK_TRUE : VK_FALSE;
        m_enabledFeatures.presentWait.presentWait = f.VK_presentWait.requested ? VK_TRUE : VK_FALSE;
        //! PResent Id comes with the present wait
        m_enabledFeatures.presentId.presentId = f.VK_presentWait.requested ? VK_TRUE : VK_FALSE;
        m_enabledFeatures.maintenance1.swapchainMaintenance1 = f.VK_maintenance1.requested ? VK_TRUE : VK_FALSE;

        // features2 chain
        m_enabledFeatures.features2.pNext = &m_enabledFeatures.vk11;
        m_enabledFeatures.vk11.pNext = &m_enabledFeatures.vk12;
        m_enabledFeatures.vk12.pNext = &m_enabledFeatures.vk13;
        m_enabledFeatures.vk13.pNext = &m_enabledFeatures.presentWait;
        m_enabledFeatures.presentWait.pNext = &m_enabledFeatures.presentId;
        m_enabledFeatures.presentId.pNext = &m_enabledFeatures.maintenance1;
        m_enabledFeatures.maintenance1.pNext = nullptr;
        m_enabledFeatures.features2.features = m_enabledFeatures.core;

        return true;
    }


    bool Device::CreateLogicalDevice(VkSurfaceKHR surface) {
        m_queueFamilyIndices = Util::FindQueueFamilies(m_physicalDevice, surface);
        CheckExit(m_queueFamilyIndices.isComplete());

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies;
        uniqueQueueFamilies.insert(m_queueFamilyIndices.graphicsFamily.value());
        if (m_queueFamilyIndices.presentFamily.value() != m_queueFamilyIndices.graphicsFamily.value())
            uniqueQueueFamilies.insert(m_queueFamilyIndices.presentFamily.value());
        if (m_queueFamilyIndices.computeFamily.value() != m_queueFamilyIndices.graphicsFamily.value())
            uniqueQueueFamilies.insert(m_queueFamilyIndices.computeFamily.value());
        if (m_queueFamilyIndices.transferFamily.value() != m_queueFamilyIndices.graphicsFamily.value())
            uniqueQueueFamilies.insert(m_queueFamilyIndices.transferFamily.value());

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo q{};
            q.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            q.queueFamilyIndex = queueFamily;
            q.queueCount = 1;
            q.pQueuePriorities = &Util::DEFAULT_QUEUE_PRIORITY;
            queueCreateInfos.push_back(q);
        }

        // Resolve device extensions (minimal for 1.3 baseline)
        auto deviceExtensions = Util::ResolveDeviceExtensions(m_required);

        // Validate extensions are present on device
        if (!Util::CheckDeviceExtensionSupport(m_physicalDevice, deviceExtensions)) {
            LogVerbose(Critical, "Required device extension missing on chosen physical device.");
            return false;
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &m_enabledFeatures.features2;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr; // using features2 chain
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();

    #if SHIFT_VALIDATION
        if (m_required.VK_enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(Util::VALIDATION_LAYERS.size());
            createInfo.ppEnabledLayerNames = Util::VALIDATION_LAYERS.data();
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.ppEnabledLayerNames = nullptr;
        }
    #else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    #endif

        if (VkCheck(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device))) {
            LogVerbose(Critical, "Failed to create logical device!");
            return false;
        }

        //! Load device extensions
        volkLoadDevice(m_device);

        // Pull queues
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily.value(), 0, &m_presentQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.computeFamily.value(), 0, &m_computeQueue);
        vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);

        Log(
            Info,
            " Device: {}. Vulkan {}. Driver version {}.",
            m_caps.version.deviceName,
            std::to_string(m_caps.version.apiVersionMajor) + std::string(".") + std::to_string(m_caps.version.apiVersionMinor),
            std::to_string(VK_VERSION_MAJOR(m_caps.version.driverVersion)) + std::string(".") + std::to_string(VK_VERSION_MINOR(m_caps.version.driverVersion))
        );

        return true;
    }

    bool Device::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, const RHIRequiredFeatures& required) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            LogVerbose(Critical, "Failed to find GPUs with Vulkan support!");
            return false;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        Util::PrintAvailablePhysicalDevices(devices);

        std::multimap<int, VkPhysicalDevice> candidates;
        for (const auto& d : devices) {
            int score = Util::RateDeviceSuitability(d, surface, required);
            candidates.insert(std::make_pair(score, d));
        }

        if (candidates.rbegin()->first > 0) {
            m_physicalDevice = candidates.rbegin()->second;
            Util::PrintDeviceName(m_physicalDevice, "Chosen device: ");

            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
            m_caps.version.apiVersionMajor = VK_VERSION_MAJOR(props.apiVersion);
            m_caps.version.apiVersionMinor = VK_VERSION_MINOR(props.apiVersion);
            m_caps.version.driverVersion = props.driverVersion;
            m_caps.version.deviceName = std::string(props.deviceName).empty() ? props.deviceName : std::string("Unknown");
        } else {
            LogVerbose(Critical, "Failed to find a suitable GPU!");
            return false;
        }

        // basic timestamp checks (kept from your code)
        VkPhysicalDeviceProperties deviceProps{};
        vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProps);
        if (deviceProps.limits.timestampPeriod == 0) {
            LogVerbose(Critical, "GPU does not support timestamp queries!");
            return false;
        }
        if (!deviceProps.limits.timestampComputeAndGraphics) {
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
            Log(Error, "Failed to create Binary VkSemaphore!");
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

    VkDescriptorSet Device::AllocateDescriptorSet(const VkDescriptorSetAllocateInfo &info, VkResult *result) const {
        VkDescriptorSet dset;
        *result = vkAllocateDescriptorSets(m_device, &info, &dset);
        return dset;
    }

    bool Device::CreateAllocator(VkInstance instance) {
        VmaVulkanFunctions vulkanFunctions = {};
        //! Some bullshit
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
        vulkanFunctions.vkFreeMemory = vkFreeMemory;
        vulkanFunctions.vkMapMemory = vkMapMemory;
        vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
        vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
        vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
        vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
        vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
        vulkanFunctions.vkCreateImage = vkCreateImage;
        vulkanFunctions.vkDestroyImage = vkDestroyImage;
        vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;

        // Vulkan 1.3
        vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
        vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
        vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
        vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;

        VmaAllocatorCreateInfo allocatorCreateInfo{};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        //! BDA-backed buffers need this, tho I hope I was not lied to
        if (m_enabledFeatures.vk12.bufferDeviceAddress == VK_TRUE) {
            allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
        allocatorCreateInfo.vulkanApiVersion = Conf::VULKAN_VERSION;
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

    void Device::ResetDescriptorPool(VkDescriptorPool pool) const {
        vkResetDescriptorPool(m_device, pool, 0);
    }
} // Shift::VK
