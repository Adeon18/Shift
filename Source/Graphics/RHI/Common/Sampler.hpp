//
// Created by otrush on 10/18/2024.
//

#ifndef SHIFT_SAMPLER_HPP
#define SHIFT_SAMPLER_HPP

#include <concepts>
#include <string>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"

namespace Shift {

    //! 1:1 with Vulkan
    enum class EFilterMode : uint8_t {
        Nearest,
        Linear
    };

    //! 1:1 with Vulkan
    enum class ESamplerAddressMode : uint8_t {
        Repeat,
        RepeatMirror,
        ClampEdge,
        ClampBorder,
        MirrorClampEdge
    };

    //! 1:1 with Vulkan
    enum class EMipMapMode : uint8_t{
        Nearest,
        Linear
    };

    //! TODO: Add support for this
//    enum class SamplerReductionMode : uint8_t {
//        Standard,
//        Comparison,
//        Minimum,
//        Maximum
//    };

    //! Base sampler description, default values for all
    struct SamplerDescriptor {
        EMipMapMode mipFilter = EMipMapMode::Nearest;
        EFilterMode minFilter = EFilterMode::Nearest;
        EFilterMode magFilter = EFilterMode::Nearest;
        ESamplerAddressMode addressModeU = ESamplerAddressMode::Repeat;
        ESamplerAddressMode addressModeV = ESamplerAddressMode::Repeat;
        ESamplerAddressMode addressModeW = ESamplerAddressMode::Repeat;

        float borderColor[4] = {0,0,0,1};
        bool unnormalizedCoordinates = false;

        bool compareEnable = false;
        ECompareOperation compareFunction = ECompareOperation::None;

        float mipLodBias = 0.0f;
        float minLod = 0.0f;
        float maxLod = 16.0f;

        float maxAnisotropy = 1.0f;

        std::string name = "EMPTY";
        //SamplerReductionMode reductionMode = SamplerReductionMode::Standard;
    };

    //! Sampler interface is very basic
    template<typename Sampler>
    concept ISampler =
        std::constructible_from<Sampler, const Device*, const SamplerDescriptor&> &&
        std::is_destructible_v<Sampler> &&
    requires (Sampler InputSampler) {
        { InputSampler.IsValid() } -> std::same_as<bool>;
    };
} // Shift

#endif //SHIFT_SAMPLER_HPP
