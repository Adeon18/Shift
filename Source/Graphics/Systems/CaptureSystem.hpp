//
// Created by otrush on 9/17/2026.
//

#ifndef SHIFT_CAPTURESYSTEM_HPP
#define SHIFT_CAPTURESYSTEM_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "Graphics/RHI/RHI.hpp"

namespace Shift::Graphics {
    //! A target to capture - we have to know the stete it was left in
    struct CaptureTarget {
        Texture* texture = nullptr;
        EResourceLayout finalLayout = EResourceLayout::ShaderReadOnlyOptimal;
        EPipelineStageFlags finalStage = EPipelineStageFlags::FragmentShaderBit;
    };

    //! One texture's mip 0 on the CPU
    struct CapturedImage {
        uint32_t width = 0;
        uint32_t height = 0;
        ETextureFormat format = ETextureFormat::UNDEFINED;
        std::vector<uint8_t> pixels;

        [[nodiscard]] bool IsValid() const { return width > 0 && height > 0 && !pixels.empty(); }
    };

    //! Reads a texture back to the CPU and writes it out
    class CaptureSystem {
    public:
        void Init(RenderBackend& backend) { m_backend = &backend; }

        //! Stall the GPU, copy mip 0 of the target back to the CPU, leave it in its final layout.
        //! \return an invalid CapturedImage if anything refused
        [[nodiscard]] CapturedImage Capture(const CaptureTarget& target) const;

        //! Write a captured image out as a PNG. Refuses any format it cannot convert
        [[nodiscard]] bool WritePNG(const CapturedImage& image, const std::string& path) const;

        //! Capture and write in one call. Empty path writes into current dir
        [[nodiscard]] bool CaptureAndWritePNG(const CaptureTarget& target, const std::string& path = "") const;

    private:
        RenderBackend* m_backend = nullptr;
    };
}

#endif //SHIFT_CAPTURESYSTEM_HPP
