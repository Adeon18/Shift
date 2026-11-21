#include "Samples.hpp"

#include <array>

namespace Shift::Samples {
    namespace {
        constexpr std::array SAMPLES{
            Sample{"00_clear", &RunClear},
            Sample{"01_triangle", &RunTriangle},
            Sample{"02_vertex_buffer", &RunVertexBuffer},
            Sample{"03_push_constants", &RunPushConstants},
            Sample{"04_indexed", &RunIndexed},
            Sample{"05_instanced", &RunInstanced},
            Sample{"06_uniform_buffer", &RunUniformBuffer},
        };
    }

    std::span<const Sample> GetSamples() {
        return SAMPLES;
    }
} // Shift::Samples
