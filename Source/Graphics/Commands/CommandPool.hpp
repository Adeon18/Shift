//
// Created by otrush on 1/30/2024.
//

#ifndef SHIFT_COMMANDPOOL_HPP
#define SHIFT_COMMANDPOOL_HPP

#include <vulkan/vulkan.h>

#include <deque>

#include "Graphics/Device/Device.hpp"
#include "Graphics/Commands/CommandBuffer.hpp"

namespace sft {
    namespace gfx {
        class CommandPool {
        public:
            enum TYPE {
                GRAPHICS,
                TRANSFER,
            };

            CommandPool(const Device& device, TYPE type);

            [[nodiscard]] bool IsValid() const { return m_commandPool != VK_NULL_HANDLE; }

            [[nodiscard]] const CommandBuffer&  RequestCommandBuffer();

            ~CommandPool();

            CommandPool() = delete;
            CommandPool(const CommandPool&) = delete;
            CommandPool& operator=(const CommandPool&) = delete;

        private:
            const Device& m_device;

            VkCommandPool m_commandPool = VK_NULL_HANDLE;
            TYPE m_type;

            std::deque<CommandBuffer> m_commandBuffers;
        };
    } // gfx
} // sft

#endif //SHIFT_COMMANDPOOL_HPP
