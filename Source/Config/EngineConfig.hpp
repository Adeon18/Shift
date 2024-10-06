#ifndef SHIFT_ENGINECONFIG_HPP
#define SHIFT_ENGINECONFIG_HPP

#include <GLFW/glfw3.h>

namespace Shift {
    namespace cfg {
        static constexpr uint32_t VULKAN_VERSION = VK_API_VERSION_1_2;

        static constexpr uint32_t DIRECTIONAL_LIGHT_MAX_COUNT = 2;
        static constexpr uint32_t POINT_LIGHT_MAX_COUNT = 6;
    }
} // shift

#endif // SHIFT_ENGINECONFIG_HPP