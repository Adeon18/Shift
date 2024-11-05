//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_SEMAPHORE_HPP
#define SHIFT_SEMAPHORE_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"

namespace Shift {
    //! Due to implementation caveats, semaphore just ahs to be created
    template<typename Semaphore>
    concept ISemaphore =
        std::is_default_constructible_v<Semaphore> &&
        std::is_copy_constructible_v<Semaphore> &&
        std::is_copy_assignable_v<Semaphore>;
} // Shift

#endif //SHIFT_SEMAPHORE_HPP
