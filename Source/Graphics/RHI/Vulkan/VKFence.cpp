#include "VKFence.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace Shift::VK {
        Fence::Fence(const Device* device, bool isSignaled): m_device(device) {
            m_fence = m_device->CreateFence(Util::CreateFenceInfo(isSignaled));
            m_valid = VkNullCheck(m_fence);
        }

        Fence::~Fence() {
            m_device->DestroyFence(m_fence);
        }

        void Fence::Wait(uint64_t limit) const {
            VkResult res =vkWaitForFences(m_device->Get(), 1, &m_fence, VK_TRUE, limit);
            if (res != VK_SUCCESS) {
                Log(Error, "Fence wait timeout!");
            }
        }

        void Fence::Reset() const {
             vkResetFences(m_device->Get(), 1, &m_fence);
        }

        VkResult Fence::Status() const {
            return vkGetFenceStatus(m_device->Get(), m_fence);
        }
} // Shift::VK