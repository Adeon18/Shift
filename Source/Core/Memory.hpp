//
// Created by otrush on 2/3/2026.
//

#ifndef SHIFT_MEMORY_HPP
#define SHIFT_MEMORY_HPP

#include <memory>

namespace Shift::Core {
    //! STL Smart pointer implementation, rewrite if another backeend is needed
    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T, typename ... Args>
    constexpr UniquePtr<T> CreateUnique(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }


    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T, typename ... Args>
    constexpr SharedPtr<T> CreateShared(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}

#endif //SHIFT_MEMORY_HPP