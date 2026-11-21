#include "Samples.hpp"

#include <array>

#include "Harness.hpp"

namespace Shift::Samples {
    namespace {
        struct PushConstants {
            float color[4];
            float offset[2];
            float scale;
            float _pad;
        };
    }

    SampleOutput RunPushConstants(SampleContext& ctx) {
        constexpr std::array DRAWS{
            PushConstants{{1.0f, 0.3f, 0.2f, 1.0f}, {-0.5f, -0.3f}, 0.6f, 0.0f},
            PushConstants{{0.2f, 0.9f, 0.3f, 1.0f}, {0.45f, -0.35f}, 0.5f, 0.0f},
            PushConstants{{0.3f, 0.4f, 1.0f, 1.0f}, {0.0f, 0.45f}, 0.8f, 0.0f},
        };

        PipelineDescriptor desc;
        desc.pushConstants = PushConstantRange{.size = sizeof(PushConstants)};

        RHI::UniquePtr<Texture> target = CreateColorTarget(ctx);
        GraphicsPipeline pipeline = CreateGraphicsPipeline(ctx.rhi, "03_PushConstants", desc);
        if (!target->IsValid() || !pipeline.IsValid()) {
            return {.error = "failed to create the target or pipeline"};
        }

        RenderContextEncoder* encoder = BeginGraphicsCmds(ctx.rhi);
        if (encoder == nullptr) {
            return {.error = "BeginCmds failed"};
        }
        BeginColorPass(*encoder, *target);
        encoder->BindGraphicsPipeline(*pipeline.pipeline);
        for (const PushConstants& draw : DRAWS) {
            encoder->SetPushConstants(*pipeline.pipeline, &draw, sizeof(draw));
            encoder->Draw({.vertexCount = 3});
        }
        encoder->EndRenderPass();

        return SubmitAndReadback(ctx.rhi, *target);
    }
} // Shift::Samples
