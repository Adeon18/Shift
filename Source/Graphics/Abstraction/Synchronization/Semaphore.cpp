#include "Semaphore.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace sft {
    namespace gfx {
        Semaphore::Semaphore(const sft::gfx::Device &device): m_device{device} {
            m_semaphore = m_device.CreateSemaphore(info::CreateSemaphoreInfo());
        }

        Semaphore::~Semaphore() {
            m_device.DestroySemaphore(m_semaphore);
        }
    }
} // sft
