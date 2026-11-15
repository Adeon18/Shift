//
// Created by otrush on 9/28/2024.
//

#ifndef SHIFT_BUFFER_HPP
#define SHIFT_BUFFER_HPP

#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

#include "Base.hpp"
#include "Sync.hpp"
#include "Types.hpp"

namespace Shift {
    //! The type of buffer that is created, NOT 1:1 with Vulkan
    enum class EBufferType {
        Uniform,
        Staging,
        Vertex,
        Index,
        Storage,
        Indirect,
        Readback
    };

    //! A buffer descriptor struct. Size is the real byte count
    struct BufferDescriptor {
        uint64_t size = 0u;
        //! Owned string on purpose
        std::string name = "EMPTY";
        EBufferType type = EBufferType::Uniform;
        //! Can a shader reach this buffer through a GPUBufferRef (DX 12 does not have ptrs)
        bool isShaderAddressable = false;
    };

    //! D3D12 packs a ref's byte offset into 32 bits and ByteAddressBuffer loads need 4-byte alignment
    inline constexpr uint64_t SHADER_REF_OFFSET_ALIGNMENT = 4;

    [[nodiscard]] constexpr bool IsValidShaderRefOffset(uint64_t bufferSize, uint64_t byteOffset) {
        return byteOffset < bufferSize &&
               byteOffset <= UINT32_MAX &&
               byteOffset % SHADER_REF_OFFSET_ALIGNMENT == 0;
    }

    //! A concept that acts as an interface for all Graphics API Buffer classes
    //! A constructor for a buffer from the RHI used standpoint should just accept the size, and for special cases some flags
    //! \tparam Buffer A Buffer class of an API backend
    template<typename Buffer>
    concept IBuffer =
        std::constructible_from<Buffer, const Device*, const BufferDescriptor&> &&
        std::is_destructible_v<Buffer> &&
    requires(Buffer InputBuffer, void* Data, uint64_t DataSize, uint64_t Offset) {
        { InputBuffer.IsValid() } -> std::same_as<bool>;
        { InputBuffer.Map() } -> std::same_as<void*>;
        { InputBuffer.GetMapped() } -> std::same_as<void*>;
        { InputBuffer.UnMap() } -> std::same_as<void>;
        { InputBuffer.InvalidateForHostRead(Offset, DataSize) } -> std::same_as<void>;
        { InputBuffer.Fill(Data, DataSize, Offset) } -> std::same_as<void>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).GetSize() } -> std::same_as<uint64_t>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).GetName() } -> std::same_as<const char *>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).IsShaderAddressable() } -> std::same_as<bool>;
        //! Vk and Metail are ptrs, dx12 is index/offset both 32 bit in one 64 bit int
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).GetShaderRef(Offset) } -> std::same_as<uint64_t>;
        { InputBuffer.GetSyncStateMutable() } -> std::same_as<BufferSyncState&>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).GetSyncState() } -> std::same_as<const BufferSyncState&>;
    };
} // Shift

#endif //SHIFT_BUFFER_HPP
