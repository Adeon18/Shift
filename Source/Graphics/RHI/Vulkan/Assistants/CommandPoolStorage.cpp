#include "CommandPoolStorage.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace Shift::VK {
    void CommandPoolStorage::Init(const Device *device, const Instance* ins) {
        m_device = device;
        m_instance = ins;

        auto queueFamiliIndices = m_device->GetQueueFamilyIndices();

        uint32_t queueFamilyIndexGraphics = queueFamiliIndices.graphicsFamily.value();
        // uint32_t queueFamilyIndexPresent = queueFamiliIndices..value();
        uint32_t queueFamilyIndexTransfer = queueFamiliIndices.transferFamily.value();

        m_graphicsPool = m_device->CreateCommandPool(Util::CreateCommandPoolInfo(queueFamilyIndexGraphics));
        m_transferPool = m_device->CreateCommandPool(Util::CreateCommandPoolInfo(queueFamilyIndexTransfer));
    }
}