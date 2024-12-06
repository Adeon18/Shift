#include "VKBuffer.hpp"

namespace Shift::VK {
    VkBufferUsageFlags BufferTypeToUsageFlags(EBufferType type) {
        VkBufferUsageFlags flags = 0;
        switch(type) {
            case EBufferType::Uniform:
                flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                break;
            case EBufferType::Staging:
                flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                break;
            case EBufferType::Vertex:
                flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                break;
            case EBufferType::Index:
                flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                break;
            //! TODO [FEATURE] need to test whether support for these buffers works
            case EBufferType::Storage:
                flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                break;
            case EBufferType::Indirect:
                flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
                break;
        }

        return flags;
    }

    VkBufferUsageFlags BufferTypeToAllocFlags(EBufferType type) {
        VmaAllocationCreateFlags flags = 0;
        switch(type) {
            case EBufferType::Staging:
            case EBufferType::Uniform:
                flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
                break;
            case EBufferType::Vertex:
            case EBufferType::Index:
                break;
            //! TODO [FEATURE] need to test whether support for these buffers works
            case EBufferType::Storage:
            case EBufferType::Indirect:
                break;
        }

        return flags;
    }

    bool Buffer::Init(const Device *device, const BufferDescriptor& desc) {
        m_device = device;
        m_desc = desc;

        VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufCreateInfo.size = m_desc.size;
        bufCreateInfo.usage = BufferTypeToUsageFlags(m_desc.type);
        bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo = {};
        //! TODO [OPTIMIZATION]: Look into PREFER_GPU/CPU flags
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = BufferTypeToAllocFlags(m_desc.type);

        if ( VkCheck(vmaCreateBuffer(m_device->GetAllocator(), &bufCreateInfo, &allocCreateInfo, &m_buffer, &m_allocation, &m_allocationInfo)) ) {
            Log(Error, "Failed to create/allocate VkBuffer!");
            return false;
        }
        return true;
    }

    void *Buffer::Map() {
        if ( VkCheck(vmaMapMemory(m_device->GetAllocator(), m_allocation, &m_allocationInfo.pMappedData)) ) {
            Log(Error, "Failed to map buffer: %s", m_desc.name);
            return nullptr;
        }
        return m_allocationInfo.pMappedData;
    }

    void Buffer::UnMap() {
        vmaUnmapMemory(m_device->GetAllocator(), m_allocation);
    }

    void Buffer::Destroy() {
        vmaDestroyBuffer(m_device->GetAllocator(), m_buffer, m_allocation);
    }
} // shift::VK
