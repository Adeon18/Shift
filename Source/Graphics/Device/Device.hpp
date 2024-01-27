//
// Created by otrush on 1/25/2024.
//

#ifndef SHIFT_DEVICE_HPP
#define SHIFT_DEVICE_HPP

#include "Utility/Vulkan/UtilVulkan.hpp"

#include "Instance.hpp"

namespace sft {
    namespace gfx {
        class Device {
        public:
            Device(const Instance &inst, VkSurfaceKHR surface);
            Device()=delete;
            Device(const Device&)=delete;
            Device& operator=(const Device&)=delete;

            [[nodiscard]] VkImageView CreateImageView(const VkImageViewCreateInfo& info) const;

            [[nodiscard]] VkFence CreateFence(const VkFenceCreateInfo& info) const;
            void DestroyFence(VkFence fence) const;

            [[nodiscard]] VkSemaphore CreateSemaphore(const VkSemaphoreCreateInfo& info) const;
            void DestroySemaphore(VkSemaphore fence) const;

            [[nodiscard]] VkDevice Get() const { return m_device; }
            [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
            [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
            [[nodiscard]] VkQueue GetPresentQueue() const { return m_presentQueue; }
            [[nodiscard]] VkQueue GetTransferQueue() const { return m_transferQueue; }

            ~Device();
        private:
            void CreateLogicalDevice(VkSurfaceKHR surface);
            void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

            VkDevice m_device = VK_NULL_HANDLE;
            VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;     // Destroyed implicitly at vkInstance destruction

            VkQueue m_graphicsQueue = VK_NULL_HANDLE;
            VkQueue m_presentQueue = VK_NULL_HANDLE;
            VkQueue m_transferQueue = VK_NULL_HANDLE;
        };
    } // gfx
} // sft

#endif //SHIFT_DEVICE_HPP
