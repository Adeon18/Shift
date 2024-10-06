#include "Semaphore.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace Shift {
    namespace gfx {
        Semaphore::Semaphore(const Shift::gfx::Device &device): m_device{device} {
            m_semaphore = m_device.CreateSemaphore(info::CreateSemaphoreInfo());
        }

        Semaphore::~Semaphore() {
            m_device.DestroySemaphore(m_semaphore);
        }
    }
} // shift
