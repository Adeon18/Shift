#include "Fence.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace shift {
    namespace gfx {
        Fence::Fence(const Device& device, bool isSignaled): m_device{device} {
            m_fence = m_device.CreateFence(info::CreateFenceInfo(isSignaled));
        }

        Fence::~Fence() {
            m_device.DestroyFence(m_fence);
        }

        void Fence::Wait(uint64_t limit) const {
            vkWaitForFences(m_device.Get(), 1, &m_fence, VK_TRUE, limit);
        }

        void Fence::Reset() const {
            vkResetFences(m_device.Get(), 1, &m_fence);
        }

        VkResult Fence::Status() const {
            return vkGetFenceStatus(m_device.Get(), m_fence);
        }
    } // gfx
} // shift