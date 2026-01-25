//
// Created by otrush on 1/15/2026.
//

#ifndef SHIFT_ITEXTURELOADER_HPP
#define SHIFT_ITEXTURELOADER_HPP

#include <optional>
#include <string>
#include <vector>
#include <array>

#include "Graphics/RHI/Common/TextureFormat.hpp"

namespace Shift {

    struct RawTextureData {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        uint32_t channels = 4;
        ETextureFormat format = ETextureFormat::UNDEFINED;
        bool isCubemap = false;
        std::vector<uint8_t> data;
        std::vector<uint32_t> mipOffsets;
    };

    class ITextureLoader {
    public:
        static constexpr std::array<uint8_t, 4> PLACEHOLDER_COLOR{0, 255, 0, 255};
        virtual ~ITextureLoader() = default;
        virtual std::optional<RawTextureData> LoadFromFile(const std::string& path) = 0;
        virtual std::optional<RawTextureData> CreatePlaceholderTexture() = 0;
    };
}

#endif //SHIFT_ITEXTURELOADER_HPP