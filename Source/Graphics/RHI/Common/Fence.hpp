//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_FENCE_HPP
#define SHIFT_FENCE_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"

namespace Shift {
    //! Fence interface, includes only base functions each implementation will have
    template<typename Fence>
    concept IFence =
        std::constructible_from<Fence, const Device*, bool> &&
        std::is_destructible_v<Fence> &&
    requires(Fence InputFence, uint64_t Limit) {
        { InputFence.Wait(Limit) } -> std::same_as<void>;
        { InputFence.Reset() } -> std::same_as<void>;
        { InputFence.IsValid() } -> std::same_as<bool>;
        { InputFence.Status() };
    };
} // Shift

#endif //SHIFT_FENCE_HPP
