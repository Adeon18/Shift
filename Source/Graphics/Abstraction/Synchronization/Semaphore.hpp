#ifndef SHIFT_SEMAPHORE_HPP
#define SHIFT_SEMAPHORE_HPP

#include <vulkan/vulkan.h>

#include "Graphics/Abstraction/Device/Device.hpp"

namespace Shift {
    namespace gfx {
        class Semaphore {
        public:
            Semaphore(const Device& device);
            ~Semaphore();

            [[nodiscard]] VkSemaphore Get() const { return m_semaphore; }
            [[nodiscard]] bool IsValid() const { return m_semaphore != VK_NULL_HANDLE; }

            Semaphore() = delete;
            Semaphore(const Semaphore&) = delete;
            Semaphore& operator=(const Semaphore&) = delete;

        private:
            const Device& m_device;

            VkSemaphore m_semaphore;
        };
    } // gfx
} // shift

#endif //SHIFT_SEMAPHORE_HPP
