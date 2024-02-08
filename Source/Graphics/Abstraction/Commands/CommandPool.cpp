#include "CommandPool.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"
#include "Utility/Vulkan/UtilVulkan.hpp"

namespace sft {
    namespace gfx {
        CommandPool::CommandPool(const sft::gfx::Device &device, POOL_TYPE type): m_device{device}, m_type{type} {
            uint32_t queueFamilyIndex = 0;
            auto queueFamiliIndices = m_device.GetQueueFamilyIndices();

            switch (m_type) {
                case POOL_TYPE::GRAPHICS:
                    queueFamilyIndex = queueFamiliIndices.graphicsFamily.value();
                    break;
                case POOL_TYPE::TRANSFER:
                    queueFamilyIndex = queueFamiliIndices.transferFamily.value();
                    break;
            }

            m_commandPool = m_device.CreateCommandPool(info::CreateCommandPoolInfo(queueFamilyIndex));
        }

        const CommandBuffer& CommandPool::RequestCommandBuffer(BUFFER_TYPE type, uint32_t frameIdx) {
            auto& buf = RequestCommandBufferManual(type, frameIdx);
            buf.BeginCommandBuffer();
            return buf;
        }

        const CommandBuffer& CommandPool::RequestCommandBufferManual(BUFFER_TYPE type, uint32_t frameIdx) {
            bool bufferInFlight = (type == BUFFER_TYPE::FLIGHT);

            uint32_t flightBuffersTaken = 0;
            for (auto& buff: m_commandBuffers[type]) {
                if (buff.IsAvailable()) {
                    buff.ResetFence();
                    return buff;
                } else if (bufferInFlight && !buff.IsAvailable()) {
                    ++flightBuffersTaken;
                }
            }

            // TODO: This is shit and should be moved to separate function
            if (flightBuffersTaken == gutil::SHIFT_MAX_FRAMES_IN_FLIGHT) {
                m_commandBuffers[type][frameIdx].Wait();
                m_commandBuffers[type][frameIdx].ResetFence();
                return m_commandBuffers[type][frameIdx];
            }

            auto& placedBuf = m_commandBuffers[type].emplace_back(m_device, m_commandPool, m_type);

            spdlog::debug("Created new buffer. Number {}, Type: {}", m_commandBuffers.size(), static_cast<int>(type));

            return placedBuf;
        }


        CommandPool::~CommandPool() {
            m_device.DestroyCommandPool(m_commandPool);
        }
    }
}