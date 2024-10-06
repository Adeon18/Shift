#ifndef SHIFT_FENCE_HPP
#define SHIFT_FENCE_HPP

#include <vulkan/vulkan.h>

#include "Graphics/Abstraction/Device/Device.hpp"

namespace Shift {
    namespace gfx {
        class Fence {
        public:
            Fence(const Device& m_device, bool isSignaled);
            ~Fence();

            [[nodiscard]] VkFence Get() const { return m_fence; }
            [[nodiscard]] bool IsValid() const { return m_fence != VK_NULL_HANDLE; }

            //! Wait till fence is signalled with timeout
            void Wait(uint64_t limit = UINT64_MAX) const;
            //! Reset the fence to be unsignaled
            void Reset() const;
            //! Get Fence status as VkResult
            [[nodiscard]] VkResult Status() const;

            Fence() = delete;
            Fence(const Fence&) = delete;
            Fence& operator=(const Fence&) = delete;
        private:
            const Device& m_device;

            VkFence m_fence;
        };
    } // gfx
} // shift

#endif //SHIFT_FENCE_HPP
