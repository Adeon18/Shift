#include "VKSemaphore.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace Shift::VK {
    bool BinarySemaphore::Init(const Device *device) {
        m_device = device;
        m_semaphore = m_device->CreateSemaphore(Util::CreateSemaphoreInfo());
        return VkNullCheck(m_semaphore);
    }

    void BinarySemaphore::Destroy() {
        m_device->DestroySemaphore(m_semaphore);
    }

    bool TimelineSemaphore::Init(const Device *device, uint64_t initialValue) {
        m_device = device;

        VkSemaphoreTypeCreateInfo info = Util::CreateTimelineSemaphoreInfo(initialValue);
        m_semaphore = m_device->CreateSemaphore(Util::CreateSemaphoreInfo(info));
        return VkNullCheck(m_semaphore);
    }

    uint64_t TimelineSemaphore::GetCurrentValue() const {
        uint64_t value;
        vkGetSemaphoreCounterValue(m_device->Get(), m_semaphore, &value);
        return value;
    }

    void TimelineSemaphore::Wait(uint64_t value) {
        VkSemaphoreWaitInfo waitInfo{};
        waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        waitInfo.flags = 0;
        waitInfo.semaphoreCount = 1;
        waitInfo.pSemaphores = &m_semaphore;
        waitInfo.pValues = &value;

        vkWaitSemaphores(m_device->Get(), &waitInfo, UINT64_MAX);
    }

    void TimelineSemaphore::Destroy() {
        m_device->DestroySemaphore(m_semaphore);
    }
} // Shift::VK
