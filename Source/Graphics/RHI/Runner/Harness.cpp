#include "Harness.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>

#include "stb_image.h"
#include "stb_image_write.h"
#ifdef _MSC_VER
#pragma warning(push)
//! FLIP issue and it produces workings and me no likey
#pragma warning(disable : 4244)
#endif
#include "FLIP/FLIP.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Shift::Samples {
    namespace {
        bool IsRGBA8(ETextureFormat format) {
            return format == ETextureFormat::R8G8B8A8_UNORM || format == ETextureFormat::R8G8B8A8_SRGB;
        }

        //! PNG bytes are sRGB, FLIP wants linear RGB
        std::vector<float> ToLinearRGB(const SampleImage& image) {
            std::array<float, 256> table{};
            for (uint32_t i = 0; i < 256; ++i) {
                const float c = static_cast<float>(i) / 255.0f;
                table[i] = c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
            }

            const size_t pixelCount = size_t(image.width) * image.height;
            std::vector<float> rgb(pixelCount * 3);
            for (size_t i = 0; i < pixelCount; ++i) {
                rgb[i * 3 + 0] = table[image.rgba[i * 4 + 0]];
                rgb[i * 3 + 1] = table[image.rgba[i * 4 + 1]];
                rgb[i * 3 + 2] = table[image.rgba[i * 4 + 2]];
            }
            return rgb;
        }

        RHI::UniquePtr<Shader> LoadShader(RenderBackend& rhi, const char* shaderName, EShaderType type) {
            const std::filesystem::path path = std::filesystem::path(SHIFT_RHI_SAMPLE_SHADERS_DIR) /
                (std::string(shaderName) + (type == EShaderType::Vertex ? ".vs.spv" : ".ps.spv"));
            std::ifstream file(path, std::ios::binary);
            std::vector<uint8_t> bytecode{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            if (bytecode.empty()) {
                return nullptr;
            }
            return RHI::UniquePtr<Shader>{rhi.CreateInterface()->CreateShader(bytecode, {.type = type, .path = path.string()})};
        }
    }

    bool SubmitAndWait(RenderBackend& rhi, RenderContext& context) {
        if (!context.EndCmds()) {
            return false;
        }
        std::array signal{rhi.ReserveGraphicsSignalPayload()};
        if (!context.SubmitCmds({}, signal)) {
            return false;
        }
        rhi.WaitForGPU();
        return true;
    }

    RenderContextEncoder* BeginGraphicsCmds(RenderBackend& rhi) {
        RenderContext& context = rhi.GetGraphicsContext();
        context.ResetCmds();
        if (!context.BeginCmds()) {
            return nullptr;
        }
        return context.CreateCommandEncoder();
    }

    RHI::UniquePtr<Texture> CreateColorTarget(const SampleContext& ctx) {
        return RHI::UniquePtr<Texture>{ctx.rhi.CreateInterface()->CreateTexture({
            .width = ctx.width,
            .height = ctx.height,
            .format = SAMPLE_COLOR_FORMAT,
            .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::TransferSrc,
            .name = "SampleTarget",
        })};
    }

    GraphicsPipeline CreateGraphicsPipeline(RenderBackend& rhi, const char* shaderName, PipelineDescriptor desc) {
        GraphicsPipeline out;
        out.vertex = LoadShader(rhi, shaderName, EShaderType::Vertex);
        out.fragment = LoadShader(rhi, shaderName, EShaderType::Fragment);
        if (!out.vertex || !out.fragment || !out.vertex->IsValid() || !out.fragment->IsValid()) {
            return out;
        }

        desc.name = shaderName;
        desc.colorBlendConfig.attachments = {{.format = SAMPLE_COLOR_FORMAT}};
        out.pipeline.reset(rhi.CreateInterface()->CreatePipeline(desc, {
            {EShaderType::Vertex, out.vertex.get()},
            {EShaderType::Fragment, out.fragment.get()},
        }));
        return out;
    }

    RHI::UniquePtr<Buffer> CreateFilledBuffer(RenderBackend& rhi, EBufferType type, std::span<const std::byte> data, const char* name) {
        RHI::UniquePtr<Buffer> buffer{rhi.CreateInterface()->CreateBuffer({.size = data.size(), .name = name, .type = type})};
        RHI::UniquePtr<Buffer> staging{rhi.CreateInterface()->CreateBuffer({.size = data.size(), .name = "SampleStaging", .type = EBufferType::Staging})};
        if (!buffer->IsValid() || !staging->IsValid() || staging->GetMapped() == nullptr) {
            return nullptr;
        }
        std::memcpy(staging->GetMapped(), data.data(), data.size());

        RenderContextEncoder* encoder = BeginGraphicsCmds(rhi);
        if (encoder == nullptr) {
            return nullptr;
        }
        encoder->CopyBufferToBuffer({staging.get(), 0u}, {buffer.get(), 0u}, static_cast<uint32_t>(data.size()));
        if (!SubmitAndWait(rhi, rhi.GetGraphicsContext())) {
            return nullptr;
        }
        return buffer;
    }

    void BeginColorPass(RenderContextEncoder& encoder, Texture& target, const UClearColor& clear) {
        const uint32_t width = target.GetWidth();
        const uint32_t height = target.GetHeight();

        encoder.TransitionTexture(target, EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);
        RenderPassDescriptor pass;
        pass.extent = {width, height};
        pass.colorAttachments.push_back({
            .loadOperation = EAttachmentLoadOperation::Clear,
            .clearValue = {.color = clear},
        });
        std::array targets{&target};
        encoder.BeginRenderPass(pass, targets, std::nullopt);
        encoder.SetViewport({0.0f, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height), 0.0f, 1.0f});
        encoder.SetScissor({{0, 0}, {width, height}});
    }

    SampleOutput SubmitAndReadback(RenderBackend& rhi, Texture& target) {
        if (!SubmitAndWait(rhi, rhi.GetGraphicsContext())) {
            return {.error = "submit failed"};
        }
        SampleOutput out{.image = ReadbackTexture(rhi, target)};
        if (!out.image.IsValid()) {
            out.error = "readback failed";
        }
        return out;
    }

    SampleImage ReadbackTexture(RenderBackend& rhi, Texture& texture) {
        SampleImage out;
        if (!IsRGBA8(texture.GetFormat()) || !Any(texture.GetUsageFlags() & ETextureUsageFlags::TransferSrc)) {
            return out;
        }

        const uint32_t width = texture.GetWidth();
        const uint32_t height = texture.GetHeight();
        const uint64_t byteSize = uint64_t(width) * height * 4;

        RHI::UniquePtr<Buffer> readback{rhi.CreateInterface()->CreateBuffer({
            .size = byteSize,
            .name = "SampleReadback",
            .type = EBufferType::Readback,
        })};
        if (!readback->IsValid()) {
            return out;
        }

        RenderContext& context = rhi.GetCaptureContext();
        context.ResetCmds();
        if (!context.BeginCmds()) {
            return out;
        }
        RenderContextEncoder* encoder = context.CreateCommandEncoder();

        encoder->TransitionTexture(texture, EResourceLayout::TransferSrcOptimal, EPipelineStageFlags::CopyBit);
        const TextureCopyDescriptor source{
            .texture = &texture,
            .size = {width, height, 1u},
            .offset = {0, 0, 0},
            .subresourceRange = {
                .aspect = texture.GetAspect(),
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u
            }
        };
        encoder->CopyTextureToBuffer(source, {readback.get(), 0u});
        encoder->TransitionBuffer(*readback, EPipelineStageFlags::HostBit, EAccessFlags::HostReadBit);

        if (!SubmitAndWait(rhi, context)) {
            return out;
        }

        readback->InvalidateForHostRead(0, byteSize);
        const void* mapped = readback->GetMapped();
        if (mapped == nullptr) {
            return out;
        }

        out.width = width;
        out.height = height;
        out.rgba.resize(static_cast<size_t>(byteSize));
        std::memcpy(out.rgba.data(), mapped, out.rgba.size());
        return out;
    }

    bool LoadPNG(const std::filesystem::path& path, SampleImage& out) {
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (pixels == nullptr) {
            return false;
        }
        out.width = static_cast<uint32_t>(width);
        out.height = static_cast<uint32_t>(height);
        out.rgba.assign(pixels, pixels + size_t(width) * height * 4);
        stbi_image_free(pixels);
        return true;
    }

    bool WritePNG(const std::filesystem::path& path, const SampleImage& image) {
        if (!image.IsValid()) {
            return false;
        }
        return stbi_write_png(path.string().c_str(), static_cast<int>(image.width), static_cast<int>(image.height), 4, image.rgba.data(), static_cast<int>(image.width) * 4) != 0;
    }

    ImageComparison CompareImages(const SampleImage& golden, const SampleImage& output) {
        ImageComparison result;
        result.sizeMatches = golden.IsValid() && output.IsValid() && golden.width == output.width && golden.height == output.height;
        if (!result.sizeMatches) {
            return result;
        }
        //! FLIP is seconds per 1080p image in Debug, and identical images score 0, so we skip if outputs are byte-identical
        if (golden.rgba == output.rgba) {
            return result;
        }

        const std::vector<float> reference = ToLinearRGB(golden);
        const std::vector<float> test = ToLinearRGB(output);

        FLIP::Parameters parameters;
        float* errorMap = nullptr;
        FLIP::evaluate(reference.data(), test.data(), static_cast<int>(golden.width), static_cast<int>(golden.height),
                       false, parameters, true, true, result.flipMean, &errorMap);

        const size_t pixelCount = size_t(golden.width) * golden.height;
        result.errorMap.width = golden.width;
        result.errorMap.height = golden.height;
        result.errorMap.rgba.resize(pixelCount * 4);
        for (size_t i = 0; i < pixelCount; ++i) {
            for (size_t c = 0; c < 3; ++c) {
                result.errorMap.rgba[i * 4 + c] = static_cast<uint8_t>(std::clamp(errorMap[i * 3 + c], 0.0f, 1.0f) * 255.0f + 0.5f);
            }
            result.errorMap.rgba[i * 4 + 3] = 255;
        }
        delete[] errorMap;
        return result;
    }
} // Shift::Samples
