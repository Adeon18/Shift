//
// Created by otrush on 9/28/2024.
//

#ifndef SHIFT_BUFFER_HPP
#define SHIFT_BUFFER_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"

namespace Shift {
    //! The type of buffer that is created, NOT 1:1 with Vulkan
    enum class EBufferType {
        Uniform,
        Staging,
        Vertex,
        Index,
        Storage,
        Indirect
    };

    //! A buffer descriptor struct, buffer size SHOULD BE ALWAYS ALIGNED BY 16!
    struct BufferDescriptor {
        uint64_t size = 0u;
        const char* name = "EMPTY";
        EBufferType type = EBufferType::Uniform;
    };

    //! A concept that acts as an interface for all Graphics API Buffer classes
    //! A constructor for a buffer from the RHI used standpoint should just accept the size, and for special cases some flags
    //! \tparam Buffer A Buffer class of an API backend
    template<typename Buffer>
    concept IBuffer =
        std::is_default_constructible_v<Buffer> &&
        std::is_trivially_destructible_v<Buffer> &&
    requires(Buffer InputBuffer, const Device* DevicePtr, const BufferDescriptor& Desc, void* Data, uint64_t DataSize, uint64_t Offset) {
        { InputBuffer.Init(DevicePtr, Desc) } -> std::same_as<bool>;
        { InputBuffer.Destroy() } -> std::same_as<void>;
        { InputBuffer.Map() } -> std::same_as<void*>;
        { InputBuffer.GetMapped() } -> std::same_as<void*>;
        { InputBuffer.UnMap() } -> std::same_as<void>;
        { InputBuffer.Fill(Data, DataSize, Offset) } -> std::same_as<void>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).GetSize() } -> std::same_as<uint64_t>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).GetName() } -> std::same_as<const char *>;
    };
} // Shift

#endif //SHIFT_BUFFER_HPP
