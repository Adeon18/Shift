#include "Samples.hpp"

#include <array>

namespace Shift::Samples {
    namespace {
        constexpr std::array SAMPLES{
            Sample{"00_clear", &RunClear},
        };
    }

    std::span<const Sample> GetSamples() {
        return SAMPLES;
    }
} // Shift::Samples
