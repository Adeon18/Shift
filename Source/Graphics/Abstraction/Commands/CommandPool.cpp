#include "CommandPool.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"
#include "Utility/Vulkan/UtilVulkan.hpp"

#include <iostream>

namespace shift {
    namespace gfx {
        CommandPool::CommandPool(const shift::gfx::Device &device, const Instance& ins, POOL_TYPE type): m_device{device}, m_instance{ins}, m_type{type} {
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
            // BIIIIIIG TODO: A HUGE PIECE OF ASS AND THIS IS JUST FOR CORRECT DEVELOPMENT WHILE I AM STUPID
            for (uint32_t i = 0; i < m_commandBuffers[type].size(); ++i) {
                if (bufferInFlight) {
                    if (frameIdx >= m_commandBuffers[type].size()) {
                        auto& placedBuf = m_commandBuffers[BUFFER_TYPE::FLIGHT].emplace_back(m_device, m_instance, m_commandPool, m_type);
                        return placedBuf;
                    }
                    if (!m_commandBuffers[type][frameIdx].IsAvailable()) m_commandBuffers[type][frameIdx].Wait();
                    m_commandBuffers[type][frameIdx].ResetFence();
                    return m_commandBuffers[type][frameIdx];
                }

                if (m_commandBuffers[type][i].IsAvailable()) {
                    m_commandBuffers[type][i].ResetFence();
                    return m_commandBuffers[type][i];
                } else if (bufferInFlight && !m_commandBuffers[type][i].IsAvailable()) {
                    ++flightBuffersTaken;
                }
            }

            // TODO: This is shit and should be moved to separate function
            if (flightBuffersTaken == gutil::SHIFT_MAX_FRAMES_IN_FLIGHT) {
                m_commandBuffers[type][frameIdx].Wait();
                m_commandBuffers[type][frameIdx].ResetFence();
                return m_commandBuffers[type][frameIdx];
            }

            auto& placedBuf = m_commandBuffers[type].emplace_back(m_device, m_instance, m_commandPool, m_type);


            spdlog::debug("Created new buffer. Number {}, Type: {}", m_commandBuffers[type].size(), static_cast<int>(type));

            return placedBuf;
        }


        CommandPool::~CommandPool() {
            m_device.DestroyCommandPool(m_commandPool);
        }
    }
}