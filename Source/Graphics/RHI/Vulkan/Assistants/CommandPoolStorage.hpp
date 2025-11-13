//
// Created by otrush on 1/30/2024.
//

#ifndef SHIFT_COMMANDPOOL_HPP
#define SHIFT_COMMANDPOOL_HPP

#include "Graphics/RHI/Vulkan/VKDevice.hpp"
#include "Graphics/RHI/Vulkan/VKCommandBuffer.hpp"

namespace Shift::VK {
    class CommandPoolStorage {
    public:
        void Init(const Device *device, const Instance* ins);
        void Destroy();

        [[nodiscard]] const Instance* GetInstance() const { return m_instance ; }

        [[nodiscard]] VkCommandPool GetGraphics() { return m_graphicsPool; }
        // [[nodiscard]] VkCommandPool GetCompute() { return m_computePool; }
        [[nodiscard]] VkCommandPool GetTransfer() { return m_transferPool; }
    private:
        const Device* m_device;
        const Instance* m_instance;

        VkCommandPool m_transferPool;
        VkCommandPool m_graphicsPool;
        // VkCommandPool m_computePool;
    };
} // Shift::VK

#endif //SHIFT_COMMANDPOOL_HPP
