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
        Offset2D()=default;
        Offset2D(const Offset2D&)=default;
        Offset2D& operator=(const Offset2D&)=default;
        Offset2D(const VkOffset2D& vkOff): x{vkOff.x}, y{vkOff.y} {}
        Offset2D(int32_t xx, int32_t yy): x{xx}, y{yy} {}

        int32_t x = 0u;
        int32_t y = 0u;
    };

    struct Extent2D {
        Extent2D()=default;
        Extent2D(const Extent2D&)=default;
        Extent2D& operator=(const Extent2D&)=default;
        Extent2D(const VkExtent2D& vkEx): x{vkEx.width}, y{vkEx.height} {}

        uint32_t x = 0u;
        uint32_t y = 0u;
    };

    struct Rect2D {
        Rect2D()=default;
        Rect2D(const Rect2D&)=default;
        Rect2D& operator=(const Rect2D&)=default;
        Rect2D(const VkRect2D& vkRec): offset{vkRec.offset}, extent{vkRec.extent} {}

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
#define DEFINE_ENUM_CLASS_BITWISE_OPERATORS(Enum)                   \
    inline constexpr Enum operator|(Enum Lhs, Enum Rhs) {           \
        return static_cast<Enum>(                                   \
            static_cast<std::underlying_type_t<Enum>>(Lhs) |        \
            static_cast<std::underlying_type_t<Enum>>(Rhs));        \
    }                                                               \
    inline constexpr Enum operator&(Enum Lhs, Enum Rhs) {           \
        return static_cast<Enum>(                                   \
            static_cast<std::underlying_type_t<Enum>>(Lhs) &        \
            static_cast<std::underlying_type_t<Enum>>(Rhs));        \
    }                                                               \
    inline constexpr Enum operator^(Enum Lhs, Enum Rhs) {           \
        return static_cast<Enum>(                                   \
            static_cast<std::underlying_type_t<Enum>>(Lhs) ^        \
            static_cast<std::underlying_type_t<Enum>>(Rhs));        \
    }                                                               \
    inline constexpr Enum operator~(Enum E) {                       \
        return static_cast<Enum>(                                   \
            ~static_cast<std::underlying_type_t<Enum>>(E));         \
    }                                                               \
    inline Enum& operator|=(Enum& Lhs, Enum Rhs) {                  \
        return Lhs = static_cast<Enum>(                             \
                   static_cast<std::underlying_type_t<Enum>>(Lhs) | \
                   static_cast<std::underlying_type_t<Enum>>(Lhs)); \
    }                                                               \
    inline Enum& operator&=(Enum& Lhs, Enum Rhs) {                  \
        return Lhs = static_cast<Enum>(                             \
                   static_cast<std::underlying_type_t<Enum>>(Lhs) & \
                   static_cast<std::underlying_type_t<Enum>>(Lhs)); \
    }                                                               \
    inline Enum& operator^=(Enum& Lhs, Enum Rhs) {                  \
        return Lhs = static_cast<Enum>(                             \
                   static_cast<std::underlying_type_t<Enum>>(Lhs) ^ \
                   static_cast<std::underlying_type_t<Enum>>(Lhs)); \
    }

#endif //SHIFT_BASE_HPP
