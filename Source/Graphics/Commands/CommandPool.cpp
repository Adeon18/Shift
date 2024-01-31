#include "CommandPool.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace sft {
    namespace gfx {
        CommandPool::CommandPool(const sft::gfx::Device &device, TYPE type): m_device{device}, m_type{type} {
            uint32_t queueFamilyIndex = 0;
            auto queueFamiliIndices = m_device.GetQueueFamilyIndices();

            switch (m_type) {
                case GRAPHICS:
                    queueFamilyIndex = queueFamiliIndices.graphicsFamily.value();
                    break;
                case TRANSFER:
                    queueFamilyIndex = queueFamiliIndices.transferFamily.value();
                    break;
            }

            m_commandPool = m_device.CreateCommandPool(info::CreateCommandPoolInfo(queueFamilyIndex));
        }

        const CommandBuffer& CommandPool::RequestCommandBuffer() {
            for (auto& buff: m_commandBuffers) {
                if (buff.IsAvailable()) {
                    buff.ResetFence();
                    return buff;
                }
            }

            m_commandBuffers.emplace_back(m_device, m_commandPool);

            spdlog::trace("Created new buffer. Number {}", m_commandBuffers.size());

            return m_commandBuffers.back();
        }

        CommandPool::~CommandPool() {
            m_device.DestroyCommandPool(m_commandPool);
        }
    }
}