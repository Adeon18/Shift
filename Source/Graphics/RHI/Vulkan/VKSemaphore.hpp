#ifndef SHIFT_VKSEMAPHORE_HPP
#define SHIFT_VKSEMAPHORE_HPP

#include "VKDevice.hpp"
#include "Graphics/RHI/Common/Semaphore.hpp"
#include "Utility/Vulkan/VKDebugUtils.hpp"

namespace Shift::VK {
    class BinarySemaphore {
    public:
        //! Initialize a VKSemaphore
        //! \param device The device wrapper ptr
        //! \return false if failed to initialize
        BinarySemaphore(const Device* device);
        BinarySemaphore(const BinarySemaphore&)=delete;
        BinarySemaphore& operator=(const BinarySemaphore&)=delete;

        //! TODO [FIX] VK_
        [[nodiscard]] VkSemaphore Get() const { return m_semaphore; }
        [[nodiscard]] const VkSemaphore* Ptr() const { return &m_semaphore; }

        [[nodiscard]] bool IsValid() const {return m_valid;}

        void SetDebugName(const char* name) const { Util::SetDebugName(m_device->Get(), VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(m_semaphore), name); }

        //! Free the VkSemaphore
        ~BinarySemaphore();
    private:
        const Device* m_device = nullptr;

        bool m_valid = false;

        VkSemaphore m_semaphore = VK_NULL_HANDLE;
    };

    class TimelineSemaphore {
    public:
        //! Initialize a VKSemaphore
        //! \param device The device wrapper ptr
        //! \return false if failed to initialize
        TimelineSemaphore(const Device* device, uint64_t initialValue);
        TimelineSemaphore(const TimelineSemaphore&)=delete;
        TimelineSemaphore& operator=(const TimelineSemaphore&)=delete;

        //! TODO [FIX] VK_
        [[nodiscard]] VkSemaphore Get() const { return m_semaphore; }
        [[nodiscard]] const VkSemaphore* Ptr() const { return &m_semaphore; }

        [[nodiscard]] uint64_t GetCurrentValue() const;
        [[nodiscard]] bool IsValid() const {return m_valid;}

        //! Attach a debug-utils name; no-op without validation/debug-utils
        void SetDebugName(const char* name) const { Util::SetDebugName(m_device->Get(), VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(m_semaphore), name); }

        void Wait(uint64_t value);

        //! Free the VkSemaphore
        ~TimelineSemaphore();
    private:
        const Device* m_device = nullptr;
        bool m_valid = false;

        VkSemaphore m_semaphore = VK_NULL_HANDLE;
    };

    ASSERT_INTERFACE(ISemaphore, BinarySemaphore);
    ASSERT_INTERFACE(ISemaphore, TimelineSemaphore);
} // Shift::VK

#endif //SHIFT_VKSEMAPHORE_HPP
