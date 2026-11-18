#ifndef SHIFT_RHICONFIG_HPP
#define SHIFT_RHICONFIG_HPP

#include <cstdint>

//! Overridable from the build
#ifndef SHIFT_RHI_MAX_FRAMES_IN_FLIGHT
#define SHIFT_RHI_MAX_FRAMES_IN_FLIGHT 2
#endif

namespace Shift::RHI {
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = SHIFT_RHI_MAX_FRAMES_IN_FLIGHT;
    static constexpr uint32_t MAX_SECONDARY_CONTEXTS = 6;
    //! Two timestamps per range
    static constexpr uint32_t MAX_GPU_TIME_RANGES_PER_FRAME = 64u;
} // Shift::RHI

#endif //SHIFT_RHICONFIG_HPP
