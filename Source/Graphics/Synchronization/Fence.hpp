#ifndef SHIFT_FENCE_HPP
#define SHIFT_FENCE_HPP

#include <vulkan/vulkan.h>

#include "Graphics/Device/Device.hpp"

namespace sft {
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

            Fence() = delete;
            Fence(const Fence&) = delete;
            Fence& operator=(const Fence&) = delete;
        private:
            const Device& m_device;

            VkFence m_fence;
        };
    } // gfx
} // sft

#endif //SHIFT_FENCE_HPP
