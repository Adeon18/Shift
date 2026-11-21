#ifndef SHIFT_RHI_SAMPLE_HPP
#define SHIFT_RHI_SAMPLE_HPP

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "Graphics/RHI/RHI.hpp"

namespace Shift::Samples {
    struct SampleContext {
        RenderBackend& rhi;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    //! Tightly packed RGBA8
    struct SampleImage {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> rgba;

        [[nodiscard]] bool IsValid() const { return width > 0 && height > 0 && rgba.size() == size_t(width) * height * 4; }
    };

    //! Empty error = the sample ran
    struct SampleOutput {
        SampleImage image;
        std::string error;
    };

    struct Sample {
        const char* name;
        SampleOutput (*run)(SampleContext&);
        float flipThreshold = 0.05f;
    };

    std::span<const Sample> GetSamples();
} // Shift::Samples

#endif //SHIFT_RHI_SAMPLE_HPP
