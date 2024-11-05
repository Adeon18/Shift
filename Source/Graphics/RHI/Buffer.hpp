//
// Created by otrush on 9/28/2024.
//

#ifndef SHIFT_BUFFER_HPP
#define SHIFT_BUFFER_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"

namespace Shift {
    //! A concept that acts as an interface for all Graphics API Buffer classes
    //! A constructor for a buffer from the RHI used standpoint should just accept the size, and for special cases some flags
    //! \tparam Buffer A Buffer class of an API backend
    template<typename Buffer>
    concept IBuffer =
        std::is_default_constructible_v<Buffer> &&
        std::is_copy_constructible_v<Buffer> &&
        std::is_copy_assignable_v<Buffer> &&
        std::is_destructible_v<Buffer> &&
    requires(Buffer InputBuffer, void* Data, uint64_t DataSize) {
        { InputBuffer.Map() } -> std::same_as<void*>;
        { InputBuffer.GetMapped() } -> std::same_as<void*>;
        { InputBuffer.UnMap() } -> std::same_as<void>;
        { InputBuffer.Fill(Data, DataSize) } -> std::same_as<void*>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).Size() } -> std::same_as<uint64_t>;
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).Valid() } -> std::same_as<bool>;
        //! Get handle for a respective API
        { CONCEPT_CONST_VAR(Buffer, InputBuffer).Get() };
    };
} // Shift

#endif //SHIFT_BUFFER_HPP
