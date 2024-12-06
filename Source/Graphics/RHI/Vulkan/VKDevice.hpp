//
// Created by otrush on 1/25/2024.
//

#ifndef SHIFT_VKDEVICE_HPP
#define SHIFT_VKDEVICE_HPP

#include "vk_mem_alloc.h"

#include "Utility/Vulkan/VKUtilCore.hpp"

#include "VKInstance.hpp"

namespace Shift::VK {
    class Device {
    public:
        Device()=default;
        Device(const Device&)=delete;
        Device& operator=(const Device&)=delete;

        //! Initialize the device
        //! \param inst Instance Vulkan Wrapper
        //! \param surface Window surface
        //! \param deviceFeaturesThe physical device features that we want to have supported
        //! \return false if init failed, else true
        bool Init(const Instance &inst, VkSurfaceKHR surface, const VkPhysicalDeviceFeatures& deviceFeatures = { .samplerAnisotropy = VK_TRUE });

        //! Get the supported depth format
        //! \return Supported format
        [[nodiscard]] VkFormat FindSupportedDepthFormat() const;

        //! Create VkImageView
        //! \param info VkImageViewCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkImageView
        [[nodiscard]] VkImageView CreateImageView(const VkImageViewCreateInfo& info) const;
        //! Destroy VkImageView
        //! \param view VkImageView to destroy
        void DestroyImageView(VkImageView view) const;

        //! Create VkSampler
        //! \param info VkSamplerCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkSampler
        [[nodiscard]] VkSampler CreateImageSampler(const VkSamplerCreateInfo& info) const;
        //! Destroy VkSampler
        //! \param sampler VkSampler to destroy
        void DestroyImageSampler(VkSampler sampler) const;

        //! Create VkFence
        //! \param info VkFenceCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkFence
        [[nodiscard]] VkFence CreateFence(const VkFenceCreateInfo& info) const;
        //! Destroy VkFence
        //! \param fence VkFence to destroy
        void DestroyFence(VkFence fence) const;

        //! Create VkSemaphore
        //! \param info VkSemaphoreCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkSemaphore
        [[nodiscard]] VkSemaphore CreateSemaphore(const VkSemaphoreCreateInfo& info) const;
        //! Destroy VkSemaphore
        //! \param semaphore VkSemaphore to destroy
        void DestroySemaphore(VkSemaphore semaphore) const;

        //! Create a VkCommandPool
        //! \param info VkCommandPoolCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkCommandPool
        [[nodiscard]] VkCommandPool CreateCommandPool(const VkCommandPoolCreateInfo& info) const;
        //! Destroy a VkCommandPool
        //! \param pool VkCommandPool to destroy
        void DestroyCommandPool(VkCommandPool pool) const;

        //! Create a VkShaderModule
        //! \param info VkShaderModuleCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkShaderModule
        [[nodiscard]] VkShaderModule CreateShaderModule(const VkShaderModuleCreateInfo& info) const;
        //! Destroy a VkShaderModule
        //! \param pool VkShaderModule to destroy
        void DestroyShaderModule(VkShaderModule module) const;

        //! Create a VkRenderPass
        //! \param info VkRenderPassCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkRenderPass
        [[nodiscard]] VkRenderPass CreateRenderPass(const VkRenderPassCreateInfo& info) const;
        //! Destroy a VkRenderPass
        //! \param pool VkRenderPass to destroy
        void DestroyRenderPass(VkRenderPass pass) const;

        //! Create a VkPipeline
        //! \param info VkGraphicsPipelineCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkPipeline
        [[nodiscard]] VkPipeline CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo& info) const;
        //! Destroy a VkPipeline
        //! \param pool VkPipeline to destroy
        void DestroyPipeline(VkPipeline pipeline) const;

        //! Create a VkPipelineLayout
        //! \param info VkPipelineLayoutCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkPipelineLayout
        [[nodiscard]] VkPipelineLayout CreatePipelineLayout(const VkPipelineLayoutCreateInfo& info) const;
        //! Destroy a VkPipelineLayout
        //! \param pool VkPipelineLayout to destroy
        void DestroyPipelineLayout(VkPipelineLayout layout) const;

        //! Create a VkDescriptorSetLayout
        //! \param info VkDescriptorSetLayoutCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkDescriptorSetLayout
        [[nodiscard]] VkDescriptorSetLayout CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& info) const;
        //! Destroy a VkDescriptorSetLayout
        //! \param pool VkDescriptorSetLayout to destroy
        void DestroyDescriptorSetLayout(VkDescriptorSetLayout layout) const;

        //! Create a VkDescriptorPool
        //! \param info VkDescriptorPoolCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkDescriptorPool
        [[nodiscard]] VkDescriptorPool CreateDescriptorPool(const VkDescriptorPoolCreateInfo& info) const;
        //! Destroy a VkDescriptorPool
        //! \param pool VkDescriptorPool to destroy
        void DestroyDescriptorPool(VkDescriptorPool pool) const;

        //! Create a VkDescriptorSet
        //! \param info VkDescriptorSetAllocateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkDescriptorSet
        [[nodiscard]] VkDescriptorSet AllocateDescriptorSet(const VkDescriptorSetAllocateInfo& info) const;

        //! Create a VkFramebuffer
        //! \param info VkFramebufferCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkFramebuffer
        [[nodiscard]] VkFramebuffer CreateFrameBuffer(const VkFramebufferCreateInfo& info) const;
        //! Destroy a VkFramebuffer
        //! \param pool VkFramebuffer to destroy
        void DestroyFrameBuffer(VkFramebuffer buf) const;

        //! Create a VkQueryPool
        //! \param info VkQueryPoolCreateInfo
        //! \return VK_NULL_HANDLE if creation failed, else VkQueryPool
        [[nodiscard]] VkQueryPool CreateQueryPool(const VkQueryPoolCreateInfo& info) const;
        //! Destroy a VkQueryPool
        //! \param pool VkQueryPool to destroy
        void DestroyQueryPool(VkQueryPool pool) const;

        [[nodiscard]] VkDevice Get() const { return m_device; }
        [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
        [[nodiscard]] VkPhysicalDeviceProperties GetDeviceProperties() const { return m_deviceProperties; }
        [[nodiscard]] VmaAllocator GetAllocator() const { return m_allocator; }
        [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
        [[nodiscard]] VkQueue GetPresentQueue() const { return m_presentQueue; }
        [[nodiscard]] VkQueue GetTransferQueue() const { return m_transferQueue; }
        [[nodiscard]] const Util::QueueFamilyIndices& GetQueueFamilyIndices() const { return m_queueFamilyIndices; }

        void Destroy();
        ~Device() = default;
    private:
        //! Create a logical device
        //! \param deviceFeatures Features of a device
        //! \param surface Window surface
        //! \return false at failure
        bool CreateLogicalDevice(const VkPhysicalDeviceFeatures& deviceFeatures, VkSurfaceKHR surface);

        //! Pick the physical GPU we will run our app on
        //! \param instance VkInstance
        //! \param surface VkSurfaceKHR
        //! \return false if failed to pick a GPU with needed requirements
        bool PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

        //! Create a VMA Allocator
        //! \param instance VkInstance
        //! \return false if failed to create allocator
        bool CreateAllocator(VkInstance instance);

        VkDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties m_deviceProperties{};
        VmaAllocator m_allocator = VK_NULL_HANDLE;

        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkQueue m_transferQueue = VK_NULL_HANDLE;

        Util::QueueFamilyIndices m_queueFamilyIndices;
    };
} // Shift::VK

#endif //SHIFT_VKDEVICE_HPP
