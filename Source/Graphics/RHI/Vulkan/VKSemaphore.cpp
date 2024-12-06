#include "VKSemaphore.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace Shift::VK {
    bool Semaphore::Init(const Device *device) {
        m_device = device;
        m_semaphore = m_device->CreateSemaphore(Util::CreateSemaphoreInfo());
        return m_semaphore == VK_NULL_HANDLE;
    }

    void Semaphore::Destroy() {
        m_device->DestroySemaphore(m_semaphore);
    }
} // Shift::VK
