#include "Samples.hpp"

#include <array>

#include "Harness.hpp"

namespace Shift::Samples {
    SampleOutput RunClear(SampleContext& ctx) {
        RHI::UniquePtr<Texture> target{ctx.rhi.CreateInterface()->CreateTexture({
            .width = ctx.width,
            .height = ctx.height,
            .format = ETextureFormat::R8G8B8A8_UNORM,
            .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::TransferSrc,
            .name = "ClearTarget",
        })};
        if (!target->IsValid()) {
            return {.error = "failed to create the render target"};
        }

        RenderContext& context = ctx.rhi.GetGraphicsContext();
        context.ResetCmds();
        if (!context.BeginCmds()) {
            return {.error = "BeginCmds failed"};
        }
        RenderContextEncoder* encoder = context.CreateCommandEncoder();

        encoder->TransitionTexture(*target, EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);
        RenderPassDescriptor pass;
        pass.extent = {ctx.width, ctx.height};
        pass.colorAttachments.push_back({
            .loadOperation = EAttachmentLoadOperation::Clear,
            .clearValue = {.color = {0.2f, 0.4f, 0.8f, 1.0f}},
        });
        std::array targets{target.get()};
        encoder->BeginRenderPass(pass, targets, std::nullopt);
        encoder->EndRenderPass();

        if (!SubmitAndWait(ctx.rhi, context)) {
            return {.error = "submit failed"};
        }

        SampleOutput out{.image = ReadbackTexture(ctx.rhi, *target)};
        if (!out.image.IsValid()) {
            out.error = "readback failed";
        }
        return out;
    }
} // Shift::Samples
