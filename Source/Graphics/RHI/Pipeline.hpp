//
// Created by otrush on 10/5/2024.
//

#ifndef SHIFT_PIPELINE_HPP
#define SHIFT_PIPELINE_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"

namespace Shift {
    //! Enum for stencil operations, 1:1 with Vulkan
    enum class EStencilOp : uint8_t {
        Keep = 0,
        Zero = 1,
        Replace = 2,
        IncrementClamp = 3,
        DecrementClamp = 4,
        Invert = 5,
        IncrementWrap = 6,
        DecrementWrap = 7,
    };

    //! 1:1 with Vulkan
    enum class EPolygonMode : uint8_t {
        Fill, Line, Point,
    };

    //! 1:1 with Vulkan, we ignore bit operators here
    enum class ECullMode : uint8_t {
        None = 0,
        Front = 0b01,
        Back = 0b10,
        Both = Front | Back
    };

    //! 1:1 with Vulkan
    enum class EWindingOrder : uint8_t {
        CounterClockwise,
        Clockwise
    };

    //! 1:1 with Vulkan, needed for blend attachment state
    enum class EColorWriteMask : uint8_t {
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        RGB = Red | Green | Blue,
        RGBA = RGB | Alpha
    };

    //! Guess what? 1:1 with Vulkan too!
    enum class EBlendFactor : uint8_t {
        Zero                    = 0,
        One                     = 1,
        SourceColor             = 2,
        OneMinusSourceColor     = 3,
        DestColor               = 4,
        OneMinusDestColor       = 5,
        SourceAlpha             = 6,
        OneMinusSourceAlpha     = 7,
        DestAlpha               = 8,
        OneMinusDestAlpha       = 9,
        ConstantColor           = 10,
        OneMinusConstantColor   = 11,
        ConstantAlpha           = 12,
        OneMinusConstantAlpha   = 13,
        SourceAlphaSaturate     = 14,
        Source1Color            = 15,
        OneMinusSource1Color    = 16,
        Source1Alpha            = 17,
        OneMinusSource1Alpha    = 18
    };

    //! 1:1 with Vulkan, except skip after Max
    enum class EBlendOperation : uint8_t {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    template<typename Pipeline>
    concept IPipeline =
        std::is_default_constructible_v<Pipeline> &&
        std::is_copy_constructible_v<Pipeline> &&
        std::is_move_constructible_v<Pipeline> &&
        std::is_copy_assignable_v<Pipeline> &&
        std::is_move_assignable_v<Pipeline> &&
    requires(Pipeline InputPipeline) {
        { InputPipeline->Build() } -> std::same_as<bool>;
    };
} // Shift

#endif //SHIFT_PIPELINE_HPP
