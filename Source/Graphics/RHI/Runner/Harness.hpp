#ifndef SHIFT_RHI_HARNESS_HPP
#define SHIFT_RHI_HARNESS_HPP

#include <filesystem>
#include <string>

#include "Sample.hpp"

namespace Shift::Samples {
    [[nodiscard]] bool SubmitAndWait(RenderBackend& rhi, RenderContext& context);

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
