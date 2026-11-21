#ifndef SHIFT_RHI_HARNESS_HPP
#define SHIFT_RHI_HARNESS_HPP

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>

#include "Sample.hpp"

namespace Shift::Samples {
    inline constexpr ETextureFormat SAMPLE_COLOR_FORMAT = ETextureFormat::R8G8B8A8_UNORM;
    inline constexpr UClearColor SAMPLE_CLEAR_COLOR{.float32 = {0.1f, 0.1f, 0.1f, 1.0f}};

    struct GraphicsPipeline {
        RHI::UniquePtr<Shader> vertex;
        RHI::UniquePtr<Shader> fragment;
        RHI::UniquePtr<Pipeline> pipeline;

        [[nodiscard]] bool IsValid() const { return pipeline && pipeline->IsValid(); }
    };

    [[nodiscard]] bool SubmitAndWait(RenderBackend& rhi, RenderContext& context);

    //! Resets and begins the graphics context, nullptr on failure
    [[nodiscard]] RenderContextEncoder* BeginGraphicsCmds(RenderBackend& rhi);

    [[nodiscard]] RHI::UniquePtr<Texture> CreateColorTarget(const SampleContext& ctx);

    //! Loads <shaderName>.vs.spv and .ps.spv and sets the color attachment format
    [[nodiscard]] GraphicsPipeline CreateGraphicsPipeline(RenderBackend& rhi, const char* shaderName, PipelineDescriptor desc);

    //! Device-local buffer filled by a staging copy on its own submit
    [[nodiscard]] RHI::UniquePtr<Buffer> CreateFilledBuffer(RenderBackend& rhi, EBufferType type, std::span<const std::byte> data, const char* name);

    void BeginColorPass(RenderContextEncoder& encoder, Texture& target, const UClearColor& clear = SAMPLE_CLEAR_COLOR);

    //! Ends the graphics cmds, submits, waits and reads the target back
    [[nodiscard]] SampleOutput SubmitAndReadback(RenderBackend& rhi, Texture& target);

    //! The texture needs TransferSrc usage and an RGBA8 format. Left in TransferSrcOptimal
    [[nodiscard]] SampleImage ReadbackTexture(RenderBackend& rhi, Texture& texture);

    [[nodiscard]] bool LoadPNG(const std::filesystem::path& path, SampleImage& out);
    [[nodiscard]] bool WritePNG(const std::filesystem::path& path, const SampleImage& image);

    struct ImageComparison {
        bool sizeMatches = false;
        float flipMean = 0.0f;
        //! Magma-mapped FLIP error
        SampleImage errorMap;
    };

    [[nodiscard]] ImageComparison CompareImages(const SampleImage& golden, const SampleImage& output);
} // Shift::Samples

#endif //SHIFT_RHI_HARNESS_HPP
