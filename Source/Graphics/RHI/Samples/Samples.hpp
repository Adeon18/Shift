#ifndef SHIFT_RHI_SAMPLES_HPP
#define SHIFT_RHI_SAMPLES_HPP

#include "Sample.hpp"

namespace Shift::Samples {
    //! Input of 02_VertexBuffer.slang, also drawn by 04_Indexed
    struct ColorVertex {
        float position[2];
        float color[3];
    };
    PipelineDescriptor ColorVertexPipelineDescriptor();

    SampleOutput RunClear(SampleContext& ctx);
    SampleOutput RunTriangle(SampleContext& ctx);
    SampleOutput RunVertexBuffer(SampleContext& ctx);
    SampleOutput RunPushConstants(SampleContext& ctx);
    SampleOutput RunIndexed(SampleContext& ctx);
    SampleOutput RunInstanced(SampleContext& ctx);
    SampleOutput RunUniformBuffer(SampleContext& ctx);
} // Shift::Samples

#endif //SHIFT_RHI_SAMPLES_HPP
