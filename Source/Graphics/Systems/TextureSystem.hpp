#ifndef SHIFT_TEXTURESYSTEM_HPP
#define SHIFT_TEXTURESYSTEM_HPP

#include <unordered_map>
#include <memory>

#include "Utility/GUIDGenerator/GUIDGenerator.hpp"

#include "Graphics/Abstraction/Commands/CommandPool.hpp"
#include "Graphics/Abstraction/Images/Images.hpp"

namespace shift::gfx {
    class TextureSystem {
    public:
        TextureSystem(
                const Device& device,
                CommandPool& graphicsPool,
                CommandPool& transferPool
                );

        [[nodiscard]] SGUID LoadTexture(const std::string& path, VkFormat format, std::string name = "");
        [[nodiscard]] TextureBase* GetTexture(SGUID guid);
        [[nodiscard]] TextureBase* GetTexture(std::string name);

        [[nodiscard]] TextureBase* GetDefaultWhiteTexture() { return GetTexture("WHITE"); }
        [[nodiscard]] TextureBase* GetDefaultBlackTexture() { return GetTexture("BLACK"); }
        [[nodiscard]] TextureBase* GetDefaultGrayTexture() { return GetTexture("GRAY"); }
        [[nodiscard]] TextureBase* GetDefaultRedTexture() { return GetTexture("RED"); }
        [[nodiscard]] TextureBase* GetDefaultGreenTexture() { return GetTexture("GREEN"); }
        [[nodiscard]] TextureBase* GetDefaultBlueTexture() { return GetTexture("BLUE"); }
    private:
        void CreateDefaultTextures();

        //! Utility function for default color textures
        SGUID CreateDefaultColorTexture(const std::array<uint8_t, 4>& color, std::string name);

        const Device& m_device;
        CommandPool& m_gfxPool;
        CommandPool& m_transPool;

        std::unordered_map<std::string, SGUID> m_textureIdByName;
        std::unordered_map<SGUID, std::unique_ptr<TextureBase>> m_textures;
    };
}

#endif //SHIFT_TEXTURESYSTEM_HPP
