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
    static constexpr uint32_t MAX_BINDLESS_SAMPLERS = 8u;
    static constexpr uint32_t MAX_BINDLESS_SETS = 1u;

    //! Capacity of vertex streams
    //! rn: positions 24 MB + normals 24 MB + tangents 32 MB + uvs 16 MB
    static constexpr uint32_t MAX_SCENE_VERTICES = 2'000'000u;
    //! Index buffer, 32-bit (64 MB)
    static constexpr uint32_t MAX_SCENE_INDICES = 16'777'216u;
    static constexpr uint32_t MAX_SCENE_OBJECTS = 16384u;
    static constexpr uint32_t MAX_SCENE_MATERIALS = 1024u;

    //! Max named GPU timing ranges recorded per frame. Two timestamps per range
    static constexpr uint32_t MAX_GPU_TIME_RANGES_PER_FRAME = 64u;
}
// shift

#endif // SHIFT_ENGINECONFIG_HPP