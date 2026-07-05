#ifndef SHIFT_VKSEMAPHORE_HPP
#define SHIFT_VKSEMAPHORE_HPP

#include "VKDevice.hpp"
#include "Graphics/RHI/Common/Semaphore.hpp"
#include "Utility/Vulkan/VKDebugUtils.hpp"

namespace Shift::VK {
    //! The stage mask every binary-semaphore wait blocks at submit (sync1 pWaitDstStageMask).
    //! THis was seen duing the sync testing. Essentially when we aquire our swapchain image its latest
    //! stage is bottom of the pipe, and that does not make sence when we submit, as we should wait for
    //! color writes to be finished for the image that we want to show, rather then for the prev frame's bottom of the pipe bit
    //! So when UI stage transitions the texture with bottom of the pipe stage flags (or any pass that runs last can do that), we actually wait on color writes and do not do bott-of-pipe -> bot-of-pipe!
    //! And yes, it worked this way before lol, because the display usually finishes reading the img before the GPU can transition cuz hw is fast (and we did not run sync validation)
    //! I found a cool explanation: semaphore = "when" stage mask = "who waits for that 'when'"
    inline constexpr VkPipelineStageFlags2 BINARY_WAIT_DST_STAGES = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

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
