//
// Created by otrush on 1/30/2024.
//

#ifndef SHIFT_COMMANDPOOL_HPP
#define SHIFT_COMMANDPOOL_HPP

#include <vulkan/vulkan.h>

#include <deque>
#include <unordered_map>

#include "Graphics/Abstraction/Device/Device.hpp"
#include "CommandBuffer.hpp"

namespace Shift {
    namespace gfx {
        class CommandPool {
        public:

            CommandPool(const Shift::gfx::Device &device, const Instance& ins, POOL_TYPE type);

            [[nodiscard]] bool IsValid() const { return m_commandPool != VK_NULL_HANDLE; }

            // TODO:Temporary
            VkCommandPool Get() {return m_commandPool;}
            const Instance& GetGlobalInstance() {return m_instance;}

            //! Request command buffer, if type is specified, will look only in the respetive type, calls begin on buffer
            //! If buffer type is FLIGHT and frameIdx is UINT32_MAX the behaviour is undefined
            [[nodiscard]] const CommandBuffer& RequestCommandBuffer(BUFFER_TYPE type = BUFFER_TYPE::DEFAULT, uint32_t frameIdx = UINT32_MAX);
            //! The same, but does not call begin on buffer
            [[nodiscard]] const CommandBuffer& RequestCommandBufferManual(BUFFER_TYPE type = BUFFER_TYPE::DEFAULT, uint32_t frameIdx = UINT32_MAX);

            ~CommandPool();

            CommandPool() = delete;
            CommandPool(const CommandPool&) = delete;
            CommandPool& operator=(const CommandPool&) = delete;

        private:
            const Device& m_device;
            const Instance& m_instance;

            VkCommandPool m_commandPool = VK_NULL_HANDLE;
            POOL_TYPE m_type;

            std::unordered_map<BUFFER_TYPE, std::deque<CommandBuffer>> m_commandBuffers;
        };
    } // gfx
} // shift

#endif //SHIFT_COMMANDPOOL_HPP
