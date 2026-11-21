#include "Samples.hpp"

#include <cmath>
#include <cstring>

#include "Harness.hpp"

namespace Shift::Samples {
    namespace {
        //! Row-major, the shader does mul(v, M)
        struct Uniforms {
            float transform[4][4];
            float color[4];
        };
    }

    SampleOutput RunUniformBuffer(SampleContext& ctx) {
        constexpr float ANGLE = 0.5f;
        constexpr float SCALE = 0.8f;
        const float c = SCALE * std::cos(ANGLE);
        const float s = SCALE * std::sin(ANGLE);
        const Uniforms uniforms{
            .transform = {
                {c, s, 0.0f, 0.0f},
                {-s, c, 0.0f, 0.0f},
                {0.0f, 0.0f, 1.0f, 0.0f},
                {0.25f, -0.1f, 0.0f, 1.0f},
            },
            .color = {0.9f, 0.6f, 0.1f, 1.0f},
        };

        const PipelineLayoutDescriptor layout{.bindings = {{.binding = 0, .type = EBindingType::UniformBuffer}}};
        PipelineDescriptor desc;
        desc.descriptorLayouts = {layout};

        RHI::UniquePtr<Texture> target = CreateColorTarget(ctx);
        GraphicsPipeline pipeline = CreateGraphicsPipeline(ctx.rhi, "06_UniformBuffer", desc);
        RHI::UniquePtr<Buffer> buffer{ctx.rhi.CreateInterface()->CreateBuffer({.size = sizeof(Uniforms), .name = "SampleUniforms", .type = EBufferType::Uniform})};
        RHI::UniquePtr<ResourceSet> set{ctx.rhi.CreateInterface()->CreateResourceSet(layout)};
        if (!target->IsValid() || !pipeline.IsValid() || !buffer->IsValid() || buffer->GetMapped() == nullptr || !set->IsValid()) {
            return {.error = "failed to create the target, pipeline, buffer or set"};
        }
        std::memcpy(buffer->GetMapped(), &uniforms, sizeof(uniforms));
        set->UpdateUBO(0, *buffer);
        set->Apply();

        RenderContextEncoder* encoder = BeginGraphicsCmds(ctx.rhi);
        if (encoder == nullptr) {
            return {.error = "BeginCmds failed"};
        }
        BeginColorPass(*encoder, *target);
        encoder->BindGraphicsPipeline(*pipeline.pipeline);
        encoder->BindResourceSet(*pipeline.pipeline, 0, *set);
        encoder->Draw({.vertexCount = 3});
        encoder->EndRenderPass();

        return SubmitAndReadback(ctx.rhi, *target);
    }
} // Shift::Samples
