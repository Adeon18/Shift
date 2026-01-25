//
// Created by otrush on 1/15/2026.
//
#include "StbLoader.hpp"

#include "stb_image.h"

namespace Shift {

    std::optional<RawTextureData> StbLoader::LoadFromFile(const std::string& path) {
        int w, h, channels;
        //! TODO: [FEATURE]: One day I will support any desired channel
        stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);

        if (!pixels) return std::nullopt;

        RawTextureData data;
        data.width = static_cast<uint32_t>(w);
        data.height = static_cast<uint32_t>(h);
        //! STB Image does not know the format, the manager has to infer it from the channels
        data.channels = channels;
        data.mipLevels = 1;

        size_t size = w * h * channels;
        data.data.resize(size);
        memcpy(data.data.data(), pixels, size);
        data.mipOffsets.push_back(0);

        stbi_image_free(pixels);
        return data;
    }

    std::optional<RawTextureData> StbLoader::CreatePlaceholderTexture() {
        RawTextureData data;
        data.width = 1;
        data.height = 1;
        data.channels = 4;
        data.mipLevels = 1;
        data.data.resize(data.width * data.height * 4);
        memcpy(data.data.data(), PLACEHOLDER_COLOR.data(), 4);
        return data;
    }
}