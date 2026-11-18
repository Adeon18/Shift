#ifndef SHIFT_RHI_MEMORY_HPP
#define SHIFT_RHI_MEMORY_HPP

#include <memory>

namespace Shift::RHI {
    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T, typename ... Args>
    constexpr UniquePtr<T> CreateUnique(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
} // Shift::RHI

#endif //SHIFT_RHI_MEMORY_HPP
