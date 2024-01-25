//
// Created by otrush on 1/25/2024.
//

#ifndef SHIFT_DEVICE_HPP
#define SHIFT_DEVICE_HPP

#include "Utility/UtilVulkan.hpp"

#include "Instance.hpp"

namespace sft {
    namespace gfx {
        class Device {
        public:
            Device(const Instance &inst, VkSurfaceKHR surface);
            Device()=delete;
            Device(const Device&)=delete;
            Device& operator=(const Device&)=delete;

            [[nodiscard]] VkDevice Get() { return m_device; }
            [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; }
            [[nodiscard]] VkQueue GetGraphicsQueue() { return m_graphicsQueue; }
            [[nodiscard]] VkQueue GetPresentQueue() { return m_presentQueue; }
            [[nodiscard]] VkQueue GetTransferQueue() { return m_transferQueue; }

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
