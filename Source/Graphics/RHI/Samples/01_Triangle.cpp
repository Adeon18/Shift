#include "Samples.hpp"

#include "Harness.hpp"

namespace Shift::Samples {
    SampleOutput RunTriangle(SampleContext& ctx) {
        RHI::UniquePtr<Texture> target = CreateColorTarget(ctx);
        GraphicsPipeline pipeline = CreateGraphicsPipeline(ctx.rhi, "01_Triangle", {});
        if (!target->IsValid() || !pipeline.IsValid()) {
            return {.error = "failed to create the target or pipeline"};
        }

        RenderContextEncoder* encoder = BeginGraphicsCmds(ctx.rhi);
        if (encoder == nullptr) {
            return {.error = "BeginCmds failed"};
        }
        BeginColorPass(*encoder, *target);
        encoder->BindGraphicsPipeline(*pipeline.pipeline);
        encoder->Draw({.vertexCount = 3});
        encoder->EndRenderPass();

        return SubmitAndReadback(ctx.rhi, *target);
    }
} // Shift::Samples
