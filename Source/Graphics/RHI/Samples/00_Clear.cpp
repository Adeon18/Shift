#include "Samples.hpp"

#include "Harness.hpp"

namespace Shift::Samples {
    SampleOutput RunClear(SampleContext& ctx) {
        RHI::UniquePtr<Texture> target = CreateColorTarget(ctx);
        if (!target->IsValid()) {
            return {.error = "failed to create the render target"};
        }

        RenderContextEncoder* encoder = BeginGraphicsCmds(ctx.rhi);
        if (encoder == nullptr) {
            return {.error = "BeginCmds failed"};
        }
        BeginColorPass(*encoder, *target, {.float32 = {0.2f, 0.4f, 0.8f, 1.0f}});
        encoder->EndRenderPass();

        return SubmitAndReadback(ctx.rhi, *target);
    }
} // Shift::Samples
