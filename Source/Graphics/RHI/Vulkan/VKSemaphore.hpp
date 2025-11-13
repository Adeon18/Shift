#ifndef SHIFT_VKSEMAPHORE_HPP
#define SHIFT_VKSEMAPHORE_HPP

#include <vulkan/vulkan.h>

#include "VKDevice.hpp"
#include "Graphics/RHI/Common/Semaphore.hpp"

namespace Shift::VK {
    class BinarySemaphore {
    public:
        BinarySemaphore() = default;

        //! Initialize a VKSemaphore
        //! \param device The device wrapper ptr
        //! \return false if failed to initialize
        bool Init(const Device* device);

        //! TODO [FIX] VK_
        [[nodiscard]] VkSemaphore Get() const { return m_semaphore; }
        [[nodiscard]] const VkSemaphore* Ptr() const { return &m_semaphore; }

        //! Free the VkSemaphore
        void Destroy();
        ~BinarySemaphore() = default;
    private:
        const Device* m_device = nullptr;

        VkSemaphore m_semaphore = VK_NULL_HANDLE;
    };

    class TimelineSemaphore {
    public:
        TimelineSemaphore() = default;

        //! Initialize a VKSemaphore
        //! \param device The device wrapper ptr
        //! \return false if failed to initialize
        bool Init(const Device* device, uint64_t initialValue);

        //! TODO [FIX] VK_
        [[nodiscard]] VkSemaphore Get() const { return m_semaphore; }
        [[nodiscard]] const VkSemaphore* Ptr() const { return &m_semaphore; }

        void Wait(uint64_t value);

        //! Free the VkSemaphore
        void Destroy();
        ~TimelineSemaphore() = default;
    private:
        const Device* m_device = nullptr;

        VkSemaphore m_semaphore = VK_NULL_HANDLE;
    };

    ASSERT_INTERFACE(ISemaphore, BinarySemaphore);
} // Shift::VK

#endif //SHIFT_VKSEMAPHORE_HPP
