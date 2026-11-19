#ifndef SHIFT_ENGINECONFIG_HPP
#define SHIFT_ENGINECONFIG_HPP

#include "Graphics/RHI/Common/RHIConfig.hpp"

namespace Shift::Conf {
    static constexpr uint32_t SHIFT_MAX_FRAMES_IN_FLIGHT = RHI::MAX_FRAMES_IN_FLIGHT;
    static constexpr uint32_t MAX_BINDLESS_IMAGES = 8192u;
    static constexpr uint32_t MAX_BINDLESS_SAMPLERS = 8u;

    //! Capacity of vertex streams
    //! rn: positions 24 MB + normals 24 MB + tangents 32 MB + uvs 16 MB
    static constexpr uint32_t MAX_SCENE_VERTICES = 2'000'000u;
    //! Index buffer, 32-bit (64 MB)
    static constexpr uint32_t MAX_SCENE_INDICES = 16'777'216u;
    static constexpr uint32_t MAX_SCENE_OBJECTS = 16384u;
    //! Submesh instance list size 4b*size: (256 KB)
    static constexpr uint32_t MAX_SCENE_SUBMESH_INSTANCES = 65536u;
    static constexpr uint32_t MAX_SCENE_MATERIALS = 1024u;
    //! LightData is 64B
    static constexpr uint32_t MAX_SCENE_LIGHTS = 256u;
}
// shift

#endif // SHIFT_ENGINECONFIG_HPP