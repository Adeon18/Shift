//
// Created by otrush on 10/7/2026.
//

#ifndef SHIFT_ENUMFLAGS_HPP
#define SHIFT_ENUMFLAGS_HPP

#include <type_traits>

//! Define bitwise operators for an enum class, allowing usage as bitmasks.
//! Taken from https://voithos.io/articles/enum-class-bitmasks/
#define DEFINE_ENUM_CLASS_BITWISE_OPERATORS(Enum)                                  \
    inline constexpr Enum operator|(Enum lhs, Enum rhs) {                          \
        using T = std::underlying_type_t<Enum>;                                    \
        return static_cast<Enum>(static_cast<T>(lhs) | static_cast<T>(rhs));       \
    }                                                                              \
    inline constexpr Enum operator&(Enum lhs, Enum rhs) {                          \
        using T = std::underlying_type_t<Enum>;                                    \
        return static_cast<Enum>(static_cast<T>(lhs) & static_cast<T>(rhs));       \
    }                                                                              \
    inline constexpr Enum operator^(Enum lhs, Enum rhs) {                          \
        using T = std::underlying_type_t<Enum>;                                    \
        return static_cast<Enum>(static_cast<T>(lhs) ^ static_cast<T>(rhs));       \
    }                                                                              \
    inline constexpr Enum operator~(Enum e) {                                      \
        using T = std::underlying_type_t<Enum>;                                    \
        return static_cast<Enum>(~static_cast<T>(e));                              \
    }                                                                              \
    inline Enum& operator|=(Enum& lhs, Enum rhs) {                                 \
        using T = std::underlying_type_t<Enum>;                                    \
        lhs = static_cast<Enum>(static_cast<T>(lhs) | static_cast<T>(rhs));        \
        return lhs;                                                                \
    }                                                                              \
    inline Enum& operator&=(Enum& lhs, Enum rhs) {                                 \
        using T = std::underlying_type_t<Enum>;                                    \
        lhs = static_cast<Enum>(static_cast<T>(lhs) & static_cast<T>(rhs));        \
        return lhs;                                                                \
    }                                                                              \
    inline Enum& operator^=(Enum& lhs, Enum rhs) {                                 \
        using T = std::underlying_type_t<Enum>;                                    \
        lhs = static_cast<Enum>(static_cast<T>(lhs) ^ static_cast<T>(rhs));        \
        return lhs;                                                                \
    }                                                                              \
    inline constexpr bool Any(Enum e) {                                            \
        using T = std::underlying_type_t<Enum>;                                    \
        return static_cast<T>(e) != 0;                                             \
    }

#endif //SHIFT_ENUMFLAGS_HPP
