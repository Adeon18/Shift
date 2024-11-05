//
// Created by otrush on 10/28/2024.
//

#ifndef SHIFT_RESOURCESET_HPP
#define SHIFT_RESOURCESET_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"
#include "Pipeline.hpp"

namespace Shift {
    //! Resource Set (Descriptor set interface).
    //! Currently supports only very basic binds
    //! \tparam Set
    template<typename Set>
    concept IResourceSet =
        std::is_destructible_v<Set> &&
    requires(
            Set InputSet,
            const PipelineLayoutDescriptor& InputDescriptor,
            const Buffer& InputBuffer,
            const Texture& InputTexture,
            const Sampler& InputSampler,
            uint32_t bind,
            uint32_t offset,
            uint32_t size
    ) {
        { Set(InputDescriptor) };
        { InputSet.UpdateUBO(bind, InputBuffer) } -> std::same_as<void>;
        { InputSet.UpdateUBO(bind, InputBuffer, size, offset) } -> std::same_as<void>;
        { InputSet.UpdateTexture(bind, InputTexture) } -> std::same_as<void>;
        { InputSet.UpdateTexture(bind, InputSampler) } -> std::same_as<void>;
    };
}

#endif //SHIFT_RESOURCESET_HPP
