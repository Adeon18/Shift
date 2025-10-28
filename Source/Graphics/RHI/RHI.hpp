//
// Created by otrush on 10/28/2025.
//

#ifndef SHIFT_SRHI_HPP
#define SHIFT_SRHI_HPP

#include <concepts>

#include "Base.hpp"

namespace Shift {
    namespace RHI {
        struct Vulkan {
            static constexpr const char* Name = "Vulkan";
            // static constexpr bool SupportsBindless = true;
        };

        struct DX12 {
            static constexpr const char* Name = "DX12";
        };
    } // RHI

    // For now:D
    template<typename T>
    concept ValidAPI = std::same_as<T, RHI::Vulkan>;


    template<ValidAPI API>
    class RenderHardwareInterface {
        public:

    };
} // Shift

#endif //SHIFT_SRHI_HPP