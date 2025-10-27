#ifndef SHIFT_VKSEMAPHORE_HPP
#define SHIFT_VKSEMAPHORE_HPP

#include <vulkan/vulkan.h>

#include "VKDevice.hpp"
#include "Graphics/RHI/Semaphore.hpp"

namespace Shift::VK {
    class Semaphore {
    public:
        Semaphore() = default;
        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        //! Initialize a VKSemaphore
        //! \param device The device wrapper ptr
        //! \return false if failed to initialize
        bool Init(const Device* device);

        //! TODO [FIX] VK_
        [[nodiscard]] VkSemaphore Get() const { return m_semaphore; }
        [[nodiscard]] VkSemaphore* Ptr() const { return &m_semaphore; }

        //! Free the VkSemaphore
        void Destroy();
        ~Semaphore() = default;
    private:
        const Device* m_device = nullptr;

        VkSemaphore m_semaphore;
    };

    ASSERT_INTERFACE(ISemaphore, Semaphore);
} // Shift::VK

#endif //SHIFT_VKSEMAPHORE_HPP
