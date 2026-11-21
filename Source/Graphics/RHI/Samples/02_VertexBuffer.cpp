#include "Samples.hpp"

#include <array>
#include <cstddef>

#include "Harness.hpp"

namespace Shift::Samples {
    PipelineDescriptor ColorVertexPipelineDescriptor() {
        PipelineDescriptor desc;
        desc.vertexConfig.vertexBindings = {{.binding = 0, .stride = sizeof(ColorVertex)}};
        desc.vertexConfig.attributeDescs = {
            {.location = 0, .binding = 0, .offset = offsetof(ColorVertex, position), .format = EVertexAttributeFormat::R32G32_SignedFloat},
            {.location = 1, .binding = 0, .offset = offsetof(ColorVertex, color), .format = EVertexAttributeFormat::R32G32B32_SignedFloat},
        };
        return desc;
    }

    SampleOutput RunVertexBuffer(SampleContext& ctx) {
        constexpr std::array VERTICES{
            ColorVertex{{-0.7f, -0.5f}, {1.0f, 0.5f, 0.0f}},
            ColorVertex{{0.5f, -0.7f}, {0.0f, 1.0f, 0.5f}},
            ColorVertex{{0.2f, 0.6f}, {0.5f, 0.0f, 1.0f}},
        };

        RHI::UniquePtr<Texture> target = CreateColorTarget(ctx);
        GraphicsPipeline pipeline = CreateGraphicsPipeline(ctx.rhi, "02_VertexBuffer", ColorVertexPipelineDescriptor());
        RHI::UniquePtr<Buffer> vertices = CreateFilledBuffer(ctx.rhi, EBufferType::Vertex, std::as_bytes(std::span(VERTICES)), "SampleVertices");
        if (!target->IsValid() || !pipeline.IsValid() || !vertices) {
            return {.error = "failed to create the target, pipeline or vertex buffer"};
        }

        RenderContextEncoder* encoder = BeginGraphicsCmds(ctx.rhi);
        if (encoder == nullptr) {
            return {.error = "BeginCmds failed"};
        }
        encoder->TransitionBuffer(*vertices, EPipelineStageFlags::VertexAttributeInputBit, EAccessFlags::VertexAttributeReadBit);
        BeginColorPass(*encoder, *target);
        encoder->BindGraphicsPipeline(*pipeline.pipeline);
        encoder->BindVertexBuffer({vertices.get(), 0u}, 0);
        encoder->Draw({.vertexCount = static_cast<uint32_t>(VERTICES.size())});
        encoder->EndRenderPass();

        return SubmitAndReadback(ctx.rhi, *target);
    }
} // Shift::Samples
