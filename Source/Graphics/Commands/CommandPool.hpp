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

            CommandPool(const Device& device, POOL_TYPE type);

            [[nodiscard]] bool IsValid() const { return m_commandPool != VK_NULL_HANDLE; }

            // TODO:Temporary
            VkCommandPool Get() {return m_commandPool;}

            [[nodiscard]] const CommandBuffer& RequestCommandBuffer();
            [[nodiscard]] const CommandBuffer& RequestCommandBufferManual();

            //! Request a command buffer that is new and not stored in pool
            [[nodiscard]] CommandBuffer  RequestCommandBufferNew();

            ~CommandPool();

            CommandPool() = delete;
            CommandPool(const CommandPool&) = delete;
            CommandPool& operator=(const CommandPool&) = delete;

        private:
            const Device& m_device;

            VkCommandPool m_commandPool = VK_NULL_HANDLE;
            POOL_TYPE m_type;

            std::deque<CommandBuffer> m_commandBuffers;
        };
    } // gfx
} // sft

#endif //SHIFT_COMMANDPOOL_HPP
