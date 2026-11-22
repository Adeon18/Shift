//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_SEMAPHORE_HPP
#define SHIFT_SEMAPHORE_HPP

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"

namespace Shift {
    //! The per-queue timelines the RHI waits and signals. Binary semaphores are backend-internal
    template<typename Semaphore>
    concept ITimelineSemaphore =
        std::constructible_from<Semaphore, const Device*, uint64_t> &&
        !std::is_trivially_destructible_v<Semaphore> &&
    requires(Semaphore InputSemaphore, uint64_t value, const char* name) {
        { CONCEPT_CONST_VAR(Semaphore, InputSemaphore).IsValid() } -> std::same_as<bool>;
        { CONCEPT_CONST_VAR(Semaphore, InputSemaphore).SetDebugName(name) } -> std::same_as<void>;
        { InputSemaphore.Wait(value) } -> std::same_as<void>;
        { CONCEPT_CONST_VAR(Semaphore, InputSemaphore).GetCurrentValue() } -> std::same_as<uint64_t>;
    };
} // Shift

#endif //SHIFT_SEMAPHORE_HPP
