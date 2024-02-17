#ifndef SHIFT_BASICBUFFERS_HPP
#define SHIFT_BASICBUFFERS_HPP

#include "Graphics/Abstraction/Device/Device.hpp"

namespace sft::gfx {
    class Buffer {
    public:
        Buffer(
                const Device& device,
                uint64_t size,
                VmaAllocationCreateFlags allocFlags,
                VkBufferUsageFlags usage
            );

        [[nodiscard]] bool IsValid() const { return m_buffer != VK_NULL_HANDLE; }

        [[nodiscard]] VkBuffer Get() const { return m_buffer; }
        [[nodiscard]] VmaAllocation GetAlloc() const { return m_allocation; }
        [[nodiscard]] VmaAllocationInfo GetAllocInfo() const { return m_allocationInfo; }

        //! Get the mapped buffer ptr - if not mapped, returns nullptr
        [[nodiscard]] void* GetMappedBuffer() { return m_allocationInfo.pMappedData; }

        [[nodiscard]] void* Map();
        void Unmap();

        ~Buffer();

        Buffer() = delete;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
    private:
        const Device& m_device;

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocationInfo;
    };

    class StagingBuffer: public Buffer {
    public:
        StagingBuffer(
                const Device& device,
                uint64_t size
        ):
        Buffer(
            device,
            size,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        {}
    };
} // sft::gfx

#endif //SHIFT_BASICBUFFERS_HPP
