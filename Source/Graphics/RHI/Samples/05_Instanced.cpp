#include "Samples.hpp"

#include <array>

#include "Harness.hpp"

namespace Shift::Samples {
    SampleOutput RunInstanced(SampleContext& ctx) {
        constexpr uint32_t GRID = 4;
        constexpr std::array<float, 6> TRIANGLE{-0.15f, -0.12f, 0.15f, -0.12f, 0.0f, 0.15f};
        std::array<float, GRID * GRID * 2> offsets{};
        for (uint32_t i = 0; i < GRID * GRID; ++i) {
            offsets[i * 2 + 0] = -0.75f + 0.5f * static_cast<float>(i % GRID);
            offsets[i * 2 + 1] = -0.75f + 0.5f * static_cast<float>(i / GRID);
        }

        PipelineDescriptor desc;
        desc.vertexConfig.vertexBindings = {
            {.binding = 0, .stride = 2 * sizeof(float), .inputRate = EVertexInputRate::PerVertex},
            {.binding = 1, .stride = 2 * sizeof(float), .inputRate = EVertexInputRate::PerInstance},
        };
        desc.vertexConfig.attributeDescs = {
            {.location = 0, .binding = 0, .offset = 0, .format = EVertexAttributeFormat::R32G32_SignedFloat},
            {.location = 1, .binding = 1, .offset = 0, .format = EVertexAttributeFormat::R32G32_SignedFloat},
        };

        RHI::UniquePtr<Texture> target = CreateColorTarget(ctx);
        GraphicsPipeline pipeline = CreateGraphicsPipeline(ctx.rhi, "05_Instanced", desc);
        RHI::UniquePtr<Buffer> vertices = CreateFilledBuffer(ctx.rhi, EBufferType::Vertex, std::as_bytes(std::span(TRIANGLE)), "SampleVertices");
        RHI::UniquePtr<Buffer> instances = CreateFilledBuffer(ctx.rhi, EBufferType::Vertex, std::as_bytes(std::span(offsets)), "SampleInstances");
        if (!target->IsValid() || !pipeline.IsValid() || !vertices || !instances) {
            return {.error = "failed to create the target, pipeline or buffers"};
        }

        RenderContextEncoder* encoder = BeginGraphicsCmds(ctx.rhi);
        if (encoder == nullptr) {
            return {.error = "BeginCmds failed"};
        }
        encoder->TransitionBuffer(*vertices, EPipelineStageFlags::VertexAttributeInputBit, EAccessFlags::VertexAttributeReadBit);
        encoder->TransitionBuffer(*instances, EPipelineStageFlags::VertexAttributeInputBit, EAccessFlags::VertexAttributeReadBit);
        BeginColorPass(*encoder, *target);
        encoder->BindGraphicsPipeline(*pipeline.pipeline);
        std::array<BufferOpDescriptor, 2> buffers{{{vertices.get(), 0u}, {instances.get(), 0u}}};
        encoder->BindVertexBuffers(buffers, 0);
        encoder->Draw({.vertexCount = 3, .instanceCount = GRID * GRID});
        encoder->EndRenderPass();

        return SubmitAndReadback(ctx.rhi, *target);
    }
} // Shift::Samples
