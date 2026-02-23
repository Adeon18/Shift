#ifndef SHIFT_VKFENCE_HPP
#define SHIFT_VKFENCE_HPP

#include "VKDevice.hpp"

#include "../Common/Fence.hpp"

namespace Shift::VK {
    class Fence {
    public:
        //! Initialize a fence
        //! \param device Pointer to a device wrapper
        //! \param isSignaled whether the fence is signalled by default
        //! \return false if failed to initialized
        Fence(const Device* device, bool isSignaled);
        Fence(const Fence& ) = delete;
        Fence& operator=(const Fence& ) = delete;

        //! Get the VkFence, not RHI compatible, hence the name
        //! \return VkFence
        [[nodiscard]] VkFence Get() const { return m_fence; }

        //! Wait till fence is signalled with timeout
        //! \param limit The maximum allowed amount of time to wait
        void Wait(uint64_t limit = UINT64_MAX) const;

        //! Reset the fence to be unsignaled
        void Reset() const;

        [[nodiscard]] bool IsValid() const {return m_valid;}

        //! Get Fence status as VkResult
        //! \return VkResult as status of the fence
        [[nodiscard]] VkResult Status() const;

        //! Free the VkFence
        ~Fence();
    private:
        const Device* m_device = nullptr;

        VkFence m_fence = VK_NULL_HANDLE;
        bool m_valid = false;
    };

    ASSERT_INTERFACE(IFence, Fence);
} // Shift::VK

#endif //SHIFT_VKFENCE_HPP
