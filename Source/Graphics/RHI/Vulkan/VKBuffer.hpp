#ifndef SHIFT_VKBUFFER_HPP
#define SHIFT_VKBUFFER_HPP

#include <span>

#include "VKDevice.hpp"
#include "Graphics/RHI/Buffer.hpp"

namespace Shift::VK {
    class Buffer {
    public:
        Buffer() = default;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        //! Create a Vulkan Buffer
        //! \param device
        //! \param desc RHI Buffer description
        //! \return false if failure
        [[nodiscard]] bool Init(const Device* device, const BufferDescriptor& desc);

        //! This is NOT and RHI function and should be called ONLY in other Vulkan handles
        //! \return The VkBuffer handle
        [[nodiscard]] VkBuffer VK_Get() const { return m_buffer; }
        [[nodiscard]] uint64_t GetSize() const { return m_desc.size; }
        [[nodiscard]] const char* GetName() const { return m_desc.name; }

        //! Get the mapped buffer ptr, only works for mapped buffers at creation, or after calling Map!
        //! \return The mapped buffer pointer, nullptr if not mapped
        [[nodiscard]] void* GetMapped() { return m_allocationInfo.pMappedData; }

        //! Map the buffer, works only on buffers that were configured to be mapped. Has to be unmapped after via UnMap().
        //! \return The mapped buffer pointer, nullptr if not mapped
        [[nodiscard]] void* Map();
        //! Unmap the mapped buffer
        void UnMap();

        //! Fill buffer with data, works on MAPPED BUFFERS ONLY
        //! \tparam T data type
        //! \param data data
        //! \param size size of the passed data
        //! \param offset offset into the mapped memory
        template<typename T>
        void Fill(T* data, uint64_t size, uint64_t offset);

        void Destroy();
        ~Buffer() = default;
    private:
        const Device* m_device = nullptr;

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocationInfo{};

        BufferDescriptor m_desc{};
    };

    //! TODO [BUG/OPTIMIZATION/CLEANUP] Look at vmaCopyMemoryToAllocation()
    template<typename T>
    void Buffer::Fill(T *data, size_t size, uint64_t offset) {
        memcpy(reinterpret_cast<char*>(m_allocationInfo.pMappedData) + offset, data, size);
    }

    ASSERT_INTERFACE(IBuffer, Buffer);
} // Shift::VK

#endif //SHIFT_VKBUFFER_HPP
