#include "FrameBuffer.hpp"

namespace shift::gfx {
    FrameBuffer::FrameBuffer(const shift::gfx::Device &device, VkRenderPass pass, VkExtent2D extent,
                             std::span<VkImageView> views): m_device{device} {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pass;
        framebufferInfo.attachmentCount = views.size();
        framebufferInfo.pAttachments = views.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        m_frameBuffer = m_device.CreateFrameBuffer(framebufferInfo);
    }

    FrameBuffer::~FrameBuffer() {
        m_device.DestroyFrameBuffer(m_frameBuffer);
    }
} // shift::gfx