#include "BasicBuffers.hpp"

namespace Shift::gfx {
    Buffer::Buffer(const Shift::gfx::Device &device, uint64_t size, VmaAllocationCreateFlags allocFlags,
                   VkBufferUsageFlags usage): m_device{device}, m_size{size} {
        VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufCreateInfo.size = size;
        bufCreateInfo.usage = usage;
        bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = allocFlags;

        vmaCreateBuffer(
                m_device.GetAllocator(),
                &bufCreateInfo,
                &allocCreateInfo,
                &m_buffer,
                &m_allocation,
                &m_allocationInfo);
    }

    void *Buffer::Map() {
        void *data = nullptr;
        vmaMapMemory(m_device.GetAllocator(), m_allocation, &data);
        return data;
    }

    void Buffer::Unmap() {
        vmaUnmapMemory(m_device.GetAllocator(), m_allocation);
    }

    Buffer::~Buffer() {
        vmaDestroyBuffer(m_device.GetAllocator(), m_buffer, m_allocation);
    }
} // shift::gfx
