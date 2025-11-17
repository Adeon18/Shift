//
// Created by otrush on 9/29/2024.
//

#ifndef SHIFT_BASE_HPP
#define SHIFT_BASE_HPP

#include <type_traits>

namespace Shift {
    //! 1:1 with Vulkan
    enum class ECompareOperation : uint8_t {
        Never = 0,
        Less = 1,
        Equal = 2,
        LessOrEqual = 3,
        Greater = 4,
        NotEqual = 5,
        GreaterOrEqual = 6,
        Always = 7,
        None
    };

    struct Offset2D {
        int32_t x = 0u;
        int32_t y = 0u;
    };

    struct Extent2D {
        uint32_t x = 0u;
        uint32_t y = 0u;
    };

    struct Rect2D {
        Offset2D offset;
        Extent2D extent;
    };

    struct Offset3D {
        int32_t x = 0u;
        int32_t y = 0u;
        int32_t z = 0u;
    };

    struct Extent3D {
        uint32_t x = 0u;
        uint32_t y = 0u;
        uint32_t z = 0u;
    };

    struct Rect3D {
        Offset3D offset;
        Extent3D extent;
    };

    struct Viewport {
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
    };
}; // Shift

//! Check whether a class implements a concept interface. Useful for checking compile-time "polymorphism"
#define ASSERT_INTERFACE(ConceptInterface, ClassImpl) static_assert(ConceptInterface<ClassImpl>)

//! Ensure the variable you are working with in the concept will be const
#define CONCEPT_CONST_VAR(TempArg, TempVar) static_cast<const TempArg&>(TempVar)

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

#endif //SHIFT_BASE_HPP
