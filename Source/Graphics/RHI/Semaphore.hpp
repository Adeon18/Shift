//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_SEMAPHORE_HPP
#define SHIFT_SEMAPHORE_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"

namespace Shift {
    //! Due to implementation caveats, semaphore just has to be created
    template<typename Semaphore>
    concept ISemaphore =
        std::is_default_constructible_v<Semaphore> &&
        std::is_trivially_destructible_v<Semaphore>
} // Shift

#endif //SHIFT_SEMAPHORE_HPP
