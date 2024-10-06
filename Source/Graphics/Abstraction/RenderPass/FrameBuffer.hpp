#ifndef SHIFT_FRAMEBUFFER_HPP
#define SHIFT_FRAMEBUFFER_HPP

#include <span>

#include "Graphics/Abstraction/Device/Device.hpp"

namespace Shift::gfx {
    //! A simple RAII wrapper over a framebuffer as a separate object
    class FrameBuffer {
    public:
        FrameBuffer(const Device& device, VkRenderPass pass, VkExtent2D extent, std::span<VkImageView> views);

        [[nodiscard]] VkFramebuffer Get() const { return m_frameBuffer; }
        [[nodiscard]] bool IsValid() const { return m_frameBuffer != VK_NULL_HANDLE; }

        ~FrameBuffer();

        FrameBuffer() = delete;
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(const FrameBuffer&) = delete;

    private:
        const Device& m_device;

        VkFramebuffer m_frameBuffer;
    };
} // shift::gfx

#endif //SHIFT_FRAMEBUFFER_HPP
