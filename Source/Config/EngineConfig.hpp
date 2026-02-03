#ifndef SHIFT_ENGINECONFIG_HPP
#define SHIFT_ENGINECONFIG_HPP

//! DO NOT DELETE
#ifdef SHIFT_VULKAN_BACKEND
#include "Utility/Vulkan/VKInclude.hpp"

namespace Shift::Conf {
    static constexpr uint32_t VULKAN_VERSION = VK_API_VERSION_1_3;
}
#endif

namespace Shift::Conf {
    static constexpr uint32_t MAX_SECONDARY_CONTEXTS = 6;

    static constexpr uint32_t DIRECTIONAL_LIGHT_MAX_COUNT = 2;
    static constexpr uint32_t POINT_LIGHT_MAX_COUNT = 6;
    static constexpr uint32_t SHIFT_MAX_FRAMES_IN_FLIGHT = 2;
    static constexpr uint32_t MAX_BINDLESS_IMAGES = 8192u;
}
// shift

#endif // SHIFT_ENGINECONFIG_HPP