//
// Created by otrush on 9/17/2026.
//

#include "CaptureSystem.hpp"

#include <array>
#include <chrono>
#include <cstring>
#include <utility>

#include "stb_image_write.h"

#include "Utility/Logging/LogMacros.hpp"

namespace Shift::Graphics {
    namespace {
        //! Add a new format here whenever needed
        uint32_t BytesPerPixel(ETextureFormat format) {
            switch (format) {
                case ETextureFormat::R8G8B8A8_UNORM:
                case ETextureFormat::R8G8B8A8_SRGB:
                case ETextureFormat::B8G8R8A8_UNORM:
                case ETextureFormat::B8G8R8A8_SRGB:
                    return 4;
                case ETextureFormat::R16G16B16A16_SFLOAT:
                    return 8;
                default:
                    return 0;
            }
        }

        bool IsBGRA(ETextureFormat format) {
            return format == ETextureFormat::B8G8R8A8_UNORM || format == ETextureFormat::B8G8R8A8_SRGB;
        }
    }

    CapturedImage CaptureSystem::Capture(const CaptureTarget& target) const {
        CapturedImage out;

        if (!m_backend || !target.texture) {
            Log(Error, "CaptureSystem: capture asked for before Init, or with no texture");
            return out;
        }

        Texture& texture = *target.texture;
        const uint32_t width = texture.GetWidth();
        const uint32_t height = texture.GetHeight();
        const uint32_t bytesPerPixel = BytesPerPixel(texture.GetFormat());
        if (width == 0 || height == 0 || bytesPerPixel == 0) {
            Log(Error, "CaptureSystem: refusing a {}x{} texture of format {}", width, height, static_cast<uint32_t>(texture.GetFormat()));
            return out;
        }

        //! Texture has to be readable from creation!
        if ((texture.GetUsageFlags() & ETextureUsageFlags::TransferSrc) == ETextureUsageFlags::None) {
            Log(Error, "CaptureSystem: texture was not created with TransferSrc usage, so it cannot be a copy source");
            return out;
        }

        const uint64_t byteSize = static_cast<uint64_t>(width) * height * bytesPerPixel;
        //! BufferDescriptor asks for a 16-aligned size
        const uint64_t allocSize = (byteSize + 15ull) & ~15ull;

        //! Wait for other commands
        m_backend->WaitForGPU();

        BufferDescriptor readbackDesc;
        readbackDesc.size = allocSize;
        readbackDesc.name = "CaptureReadback";
        readbackDesc.type = EBufferType::Readback;

        Buffer* readback = m_backend->CreateInterface()->CreateBuffer(readbackDesc);
        if (!readback || !readback->IsValid()) {
            Log(Error, "CaptureSystem: failed to create a {} byte readback buffer", allocSize);
            delete readback;
            return out;
        }

        RenderContext& gContext = m_backend->GetCaptureContext();
        RenderContextEncoder* encoder = gContext.CreateCommandEncoder();

        gContext.ResetCmds();
        if (!gContext.BeginCmds()) {
            Log(Error, "CaptureSystem: failed to begin the capture command buffer");
            delete readback;
            return out;
        }

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
        const BufferOpDescriptor destination{readback, 0u};

        encoder->CopyTextureToBuffer(source, destination);
        encoder->BarrierForHostRead(destination, byteSize);
        encoder->TransitionTexture(texture, target.finalLayout, target.finalStage);

        if (!gContext.EndCmds()) {
            Log(Error, "CaptureSystem: failed to end the capture command buffer");
            delete readback;
            return out;
        }

        //! Due to wait for gpu and the fact this is run before the next render frame, it will not lock!
        std::array sigPayloads{m_backend->ReserveGraphicsSignalPayload()};
        if (!gContext.SubmitCmds({}, sigPayloads)) {
            Log(Error, "CaptureSystem: failed to submit the capture command buffer");
            delete readback;
            return out;
        }

        m_backend->WaitForGPU();
        //! Make writes visible
        readback->InvalidateForHostRead(0, allocSize);

        const void* mapped = readback->GetMapped();
        if (mapped == nullptr) {
            Log(Error, "CaptureSystem: the readback buffer came back unmapped");
            delete readback;
            return out;
        }

        out.width = width;
        out.height = height;
        out.format = texture.GetFormat();
        out.pixels.resize(static_cast<size_t>(byteSize));
        std::memcpy(out.pixels.data(), mapped, static_cast<size_t>(byteSize));

        delete readback;
        return out;
    }

    bool CaptureSystem::WritePNG(const CapturedImage& image, const std::string& path) const {
        if (!image.IsValid()) {
            Log(Error, "CaptureSystem: nothing to write, the capture is empty");
            return false;
        }

        const uint32_t bytesPerPixel = BytesPerPixel(image.format);
        if (bytesPerPixel != 4) {
            Log(Error, "CaptureSystem: format {} is not an 8-bit RGBA format, refusing to guess a "
                       "conversion - capture the display target instead",
                static_cast<uint32_t>(image.format));
            return false;
        }

        const size_t pixelCount = static_cast<size_t>(image.width) * image.height;
        if (image.pixels.size() < pixelCount * bytesPerPixel) {
            Log(Error, "CaptureSystem: {}x{} needs {} bytes but the capture holds {}",
                image.width, image.height, pixelCount * bytesPerPixel, image.pixels.size());
            return false;
        }

        std::vector<uint8_t> rgba = image.pixels;
        if (IsBGRA(image.format)) {
            for (size_t i = 0; i < pixelCount; ++i) {
                std::swap(rgba[i * 4 + 0], rgba[i * 4 + 2]);
            }
        }

        const int written = stbi_write_png(path.c_str(), static_cast<int>(image.width),
                                           static_cast<int>(image.height), 4, rgba.data(),
                                           static_cast<int>(image.width) * 4);
        if (written == 0) {
            Log(Error, "CaptureSystem: stb failed to write '{}'", path);
            return false;
        }

        Log(Info, "Screenshot written: {} ({}x{})", path, image.width, image.height);
        return true;
    }

    bool CaptureSystem::CaptureAndWritePNG(const CaptureTarget& target, const std::string& path) const {
        const CapturedImage captured = Capture(target);
        if (!captured.IsValid()) { return false; }

        std::string outPath = path;
        if (outPath.empty()) {
            const auto stamp = std::chrono::system_clock::now().time_since_epoch();
            outPath = "Shift_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stamp).count()) + ".png";
        }
        return WritePNG(captured, outPath);
    }
}
