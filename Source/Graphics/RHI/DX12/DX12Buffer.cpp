#include "DX12Buffer.hpp"

namespace Shift::DX12 {
    Buffer::Buffer(const Device* device, const BufferDescriptor& desc) { SRHI_NOT_IMPLEMENTED(); }

    bool Buffer::IsValid() const { SRHI_NOT_IMPLEMENTED(); }
    uint64_t Buffer::GetSize() const { SRHI_NOT_IMPLEMENTED(); }
    const char* Buffer::GetName() const { SRHI_NOT_IMPLEMENTED(); }
    bool Buffer::IsShaderAddressable() const { SRHI_NOT_IMPLEMENTED(); }
    uint64_t Buffer::GetShaderRef(uint64_t byteOffset) const { SRHI_NOT_IMPLEMENTED(); }

    void* Buffer::GetMapped() { SRHI_NOT_IMPLEMENTED(); }
    void* Buffer::Map() { SRHI_NOT_IMPLEMENTED(); }
    void Buffer::UnMap() { SRHI_NOT_IMPLEMENTED(); }
    void Buffer::InvalidateForHostRead(uint64_t offset, uint64_t size) const { SRHI_NOT_IMPLEMENTED(); }

    BufferSyncState& Buffer::GetSyncStateMutable() { SRHI_NOT_IMPLEMENTED(); }
    const BufferSyncState& Buffer::GetSyncState() const { SRHI_NOT_IMPLEMENTED(); }

    Buffer::~Buffer() = default;
} // Shift::DX12
