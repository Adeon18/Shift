#include "CommandPool.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

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

        const CommandBuffer& CommandPool::RequestCommandBuffer() {
            for (auto& buff: m_commandBuffers) {
                if (buff.IsAvailable()) {
                    buff.ResetFence();
                    buff.BeginCommandBuffer();
                    return buff;
                }
            }

            auto &buf = m_commandBuffers.emplace_back(m_device, m_commandPool, m_type);

            buf.BeginCommandBuffer();
            spdlog::trace("Created new buffer. Number {}", m_commandBuffers.size());

            return m_commandBuffers.back();
        }

        const CommandBuffer& CommandPool::RequestCommandBufferManual() {
            for (auto& buff: m_commandBuffers) {
                if (buff.IsAvailable()) {
                    buff.ResetFence();
                    return buff;
                }
            }

            auto &buf = m_commandBuffers.emplace_back(m_device, m_commandPool, m_type);

            spdlog::trace("Created new buffer. Number {}", m_commandBuffers.size());

            return m_commandBuffers.back();
        }

        CommandBuffer CommandPool::RequestCommandBufferNew() {
            return CommandBuffer{m_device, m_commandPool, m_type};
        }

        CommandPool::~CommandPool() {
            m_device.DestroyCommandPool(m_commandPool);
        }
    }
}