#ifndef SHIFT_DX12BUFFER_HPP
#define SHIFT_DX12BUFFER_HPP

#include <cstdint>

#include "DX12Device.hpp"
#include "../Common/Buffer.hpp"
#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    class Buffer {
    public:
        Buffer(const Device* device, const BufferDescriptor& desc);

        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] uint64_t GetSize() const;
        [[nodiscard]] const char* GetName() const;
        [[nodiscard]] bool IsShaderAddressable() const;
        [[nodiscard]] uint64_t GetShaderRef(uint64_t byteOffset = 0) const;

        [[nodiscard]] void* GetMapped();
        [[nodiscard]] void* Map();
        void UnMap();
        void InvalidateForHostRead(uint64_t offset = 0, uint64_t size = UINT64_MAX) const;

        [[nodiscard]] BufferSyncState& GetSyncStateMutable();
        [[nodiscard]] const BufferSyncState& GetSyncState() const;

        template<typename T>
        void Fill(T* data, uint64_t size, uint64_t offset) { SRHI_NOT_IMPLEMENTED(); }

        ~Buffer();
    };

    ASSERT_INTERFACE(IBuffer, Buffer);
} // Shift::DX12

#endif //SHIFT_DX12BUFFER_HPP
