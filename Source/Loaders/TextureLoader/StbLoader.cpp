//
// Created by otrush on 1/15/2026.
//
#include "StbLoader.hpp"

#include "stb_image.h"

namespace Shift {

    std::optional<RawTextureData> StbLoader::LoadFromFile(const std::string& path) {
        int w, h, fileChannels;
        //! TODO: [FEATURE]: One day I will support any desired channel
        stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &fileChannels, STBI_rgb_alpha);

        if (!pixels) return std::nullopt;

        //! This will bite my ass later for sure
        constexpr uint32_t DELIVERED_CHANNELS = 4;

        RawTextureData data;
        data.width = static_cast<uint32_t>(w);
        data.height = static_cast<uint32_t>(h);
        data.channels = DELIVERED_CHANNELS;
        data.mipLevels = 1;

        size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * DELIVERED_CHANNELS;
        data.data.resize(size);
        memcpy(data.data.data(), pixels, size);
        data.mipOffsets.push_back(0);

        stbi_image_free(pixels);
        return data;
    }

    std::optional<RawTextureData> StbLoader::Create1x1Texture(const std::array<uint8_t, 4>& rgba) {
        RawTextureData data;
        data.width = 1;
        data.height = 1;
        data.channels = 4;
        data.mipLevels = 1;
        data.data.resize(data.width * data.height * 4);
        memcpy(data.data.data(), rgba.data(), 4);
        data.mipOffsets.push_back(0);
        return data;
    }
}