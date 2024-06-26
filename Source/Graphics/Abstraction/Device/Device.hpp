//
// Created by otrush on 1/25/2024.
//

#ifndef SHIFT_DEVICE_HPP
#define SHIFT_DEVICE_HPP

#include "vk_mem_alloc.h"

#include "Utility/Vulkan/UtilVulkan.hpp"

#include "Instance.hpp"

namespace shift {
    namespace gfx {
        class Device {
        public:
            Device(const Instance &inst, VkSurfaceKHR surface);
            Device()=delete;
            Device(const Device&)=delete;
            Device& operator=(const Device&)=delete;

            [[nodiscard]] VkFormat FindSupportedDepthFormat() const;

            [[nodiscard]] VkImageView CreateImageView(const VkImageViewCreateInfo& info) const;
            void DestroyImageView(VkImageView view) const;

            [[nodiscard]] VkSampler CreateImageSampler(const VkSamplerCreateInfo& info) const;
            void DestroyImageSampler(VkSampler sampler) const;

            [[nodiscard]] VkFence CreateFence(const VkFenceCreateInfo& info) const;
            void DestroyFence(VkFence fence) const;

            [[nodiscard]] VkSemaphore CreateSemaphore(const VkSemaphoreCreateInfo& info) const;
            void DestroySemaphore(VkSemaphore fence) const;

            [[nodiscard]] VkCommandPool CreateCommandPool(const VkCommandPoolCreateInfo& info) const;
            void DestroyCommandPool(VkCommandPool pool) const;

            [[nodiscard]] VkShaderModule CreateShaderModule(const VkShaderModuleCreateInfo& info) const;
            void DestroyShaderModule(VkShaderModule module) const;

            [[nodiscard]] VkRenderPass CreateRenderPass(const VkRenderPassCreateInfo& info) const;
            void DestroyRenderPass(VkRenderPass pass) const;

            [[nodiscard]] VkPipeline CreateGraphicsPipeline(const VkGraphicsPipelineCreateInfo& info) const;
            void DestroyPipeline(VkPipeline pipeline) const;

            [[nodiscard]] VkPipelineLayout CreatePipelineLayout(const VkPipelineLayoutCreateInfo& info) const;
            void DestroyPipelineLayout(VkPipelineLayout layout) const;

            [[nodiscard]] VkDescriptorSetLayout CreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo& info) const;
            void DestroyDescriptorSetLayout(VkDescriptorSetLayout layout) const;

            [[nodiscard]] VkDescriptorPool CreateDescriptorPool(const VkDescriptorPoolCreateInfo& info) const;
            void DestroyDescriptorPool(VkDescriptorPool pool) const;

            [[nodiscard]] VkDescriptorSet AllocateDescriptorSet(const VkDescriptorSetAllocateInfo& info) const;

            [[nodiscard]] VkFramebuffer CreateFrameBuffer(const VkFramebufferCreateInfo& info) const;
            void DestroyFrameBuffer(VkFramebuffer buf) const;

            [[nodiscard]] VkQueryPool CreateQueryPool(const VkQueryPoolCreateInfo& info) const;
            void DestroyQueryPool(VkQueryPool pool) const;

            [[nodiscard]] VkDevice Get() const { return m_device; }
            [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
            [[nodiscard]] VkPhysicalDeviceProperties GetDeviceProperties() const { return m_deviceProperties; }
            [[nodiscard]] VmaAllocator GetAllocator() const { return m_allocator; }
            [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
            [[nodiscard]] VkQueue GetPresentQueue() const { return m_presentQueue; }
            [[nodiscard]] VkQueue GetTransferQueue() const { return m_transferQueue; }
            [[nodiscard]] gutil::QueueFamilyIndices GetQueueFamilyIndices() const { return m_queueFamilyIndices; }

            ~Device();
        private:
            void CreateLogicalDevice(VkSurfaceKHR surface);
            void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
            void CreateAllocator(VkInstance instance);

            VkDevice m_device = VK_NULL_HANDLE;
            VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;     // Destroyed implicitly at vkInstance destruction
            VkPhysicalDeviceProperties m_deviceProperties{};
            VmaAllocator m_allocator = VK_NULL_HANDLE;

            VkQueue m_graphicsQueue = VK_NULL_HANDLE;
            VkQueue m_presentQueue = VK_NULL_HANDLE;
            VkQueue m_transferQueue = VK_NULL_HANDLE;

            gutil::QueueFamilyIndices m_queueFamilyIndices;
        };
    } // gfx
} // shift

#endif //SHIFT_DEVICE_HPP
