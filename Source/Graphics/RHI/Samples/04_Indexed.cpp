#include "Samples.hpp"

#include <array>

#include "Harness.hpp"

namespace Shift::Samples {
    SampleOutput RunIndexed(SampleContext& ctx) {
        constexpr std::array VERTICES{
            ColorVertex{{-0.6f, -0.5f}, {1.0f, 0.2f, 0.2f}},
            ColorVertex{{0.6f, -0.5f}, {0.2f, 1.0f, 0.2f}},
            ColorVertex{{0.6f, 0.5f}, {0.2f, 0.2f, 1.0f}},
            ColorVertex{{-0.6f, 0.5f}, {1.0f, 1.0f, 0.2f}},
        };
        constexpr std::array<uint16_t, 6> INDICES{0, 1, 2, 2, 3, 0};

        RHI::UniquePtr<Texture> target = CreateColorTarget(ctx);
        GraphicsPipeline pipeline = CreateGraphicsPipeline(ctx.rhi, "02_VertexBuffer", ColorVertexPipelineDescriptor());
        RHI::UniquePtr<Buffer> vertices = CreateFilledBuffer(ctx.rhi, EBufferType::Vertex, std::as_bytes(std::span(VERTICES)), "SampleVertices");
        RHI::UniquePtr<Buffer> indices = CreateFilledBuffer(ctx.rhi, EBufferType::Index, std::as_bytes(std::span(INDICES)), "SampleIndices");
        if (!target->IsValid() || !pipeline.IsValid() || !vertices || !indices) {
            return {.error = "failed to create the target, pipeline or buffers"};
        }

        RenderContextEncoder* encoder = BeginGraphicsCmds(ctx.rhi);
        if (encoder == nullptr) {
            return {.error = "BeginCmds failed"};
        }
        encoder->TransitionBuffer(*vertices, EPipelineStageFlags::VertexAttributeInputBit, EAccessFlags::VertexAttributeReadBit);
        encoder->TransitionBuffer(*indices, EPipelineStageFlags::IndexInputBit, EAccessFlags::IndexReadBit);
        BeginColorPass(*encoder, *target);
        encoder->BindGraphicsPipeline(*pipeline.pipeline);
        encoder->BindVertexBuffer(*pipeline.pipeline, {vertices.get(), 0u}, 0);
        encoder->BindIndexBuffer({indices.get(), 0u}, EIndexSize::UInt16);
        encoder->DrawIndexed({.indexCount = static_cast<uint32_t>(INDICES.size())});
        encoder->EndRenderPass();

        return SubmitAndReadback(ctx.rhi, *target);
    }
} // Shift::Samples
