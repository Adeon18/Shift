//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_FENCE_HPP
#define SHIFT_FENCE_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"

namespace Shift {
    //! Fence interface, includes only base functions, each implementation will have
    template<typename Fence>
    concept IFence =
        std::is_default_constructible_v<Fence> &&
        std::is_copy_constructible_v<Fence> &&
        std::is_copy_assignable_v<Fence> &&
    requires(Fence InputFence, uint64_t Limit, bool IsSignalled) {
        { Fence(IsSignalled) };
        { InputFence.Wait(Limit) } -> std::same_as<void>;
        { InputFence.Reset() } -> std::same_as<void>;
        { InputFence.Status() };
    };
} // Shift

#endif //SHIFT_FENCE_HPP
