//
// Created by otrush on 1/15/2026.
//

#ifndef SHIFT_STBLOADER_HPP
#define SHIFT_STBLOADER_HPP

#include "ITextureLoader.hpp"

namespace Shift {
    class StbLoader : public ITextureLoader {
    public:
        std::optional<RawTextureData> LoadFromFile(const std::string& path) override;
        std::optional<RawTextureData> Create1x1Texture(const std::array<uint8_t, 4>& rgba) override;
    };
}

#endif //SHIFT_STBLOADER_HPP