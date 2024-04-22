#ifndef SHIFT_BASICBUFFERS_HPP
#define SHIFT_BASICBUFFERS_HPP

#include <span>

#include "Graphics/Abstraction/Device/Device.hpp"

namespace shift::gfx {
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
        [[nodiscard]] uint64_t GetSize() const { return m_size; }

        //! Get the mapped buffer ptr - if not mapped, returns nullptr
        [[nodiscard]] void* GetMappedBuffer() { return m_allocationInfo.pMappedData; }

        [[nodiscard]] void* Map();
        void Unmap();

        //! Fill buffer with data, works on MAPPED BUFFERS ONLY
        template<typename T>
        void Fill(T* data, size_t size);

        ~Buffer();

        Buffer() = delete;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
    private:
        const Device& m_device;

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocationInfo;

        uint64_t m_size;
    };

    template<typename T>
    void Buffer::Fill(T *data, size_t size) {
        memcpy(m_allocationInfo.pMappedData, data, size);
    }

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

    class VertexBuffer: public Buffer {
    public:
        VertexBuffer(
                const Device& device,
                uint64_t size
        ):
                Buffer(
                        device,
                        size,
                        0,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        {}
    };

    class IndexBuffer: public Buffer {
    public:
        IndexBuffer(
                const Device& device,
                uint64_t size
        ):
                Buffer(
                        device,
                        size,
                        0,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        {}
    };

    class UniformBuffer: public Buffer {
    public:
        UniformBuffer(
                const Device& device,
                uint64_t size
        ):
                Buffer(
                        device,
                        size,
                        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {}
    };
} // shift::gfx

#endif //SHIFT_BASICBUFFERS_HPP
