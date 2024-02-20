#ifndef SHIFT_TEXTURESYSTEM_HPP
#define SHIFT_TEXTURESYSTEM_HPP

#include <unordered_map>
#include <memory>

#include "Utility/GUIDGenerator/GUIDGenerator.hpp"

#include "Graphics/Abstraction/Commands/CommandPool.hpp"
#include "Graphics/Abstraction/Images/Images.hpp"

namespace sft::gfx {
    class TextureSystem {
    public:
        TextureSystem(
                const Device& device,
                CommandPool& graphicsPool,
                CommandPool& transferPool
                );

        [[nodiscard]] SGUID LoadTexture(const std::string& path, VkFormat format);
        [[nodiscard]] TextureBase* GetTexture(SGUID guid);
    private:
        const Device& m_device;
        CommandPool& m_gfxPool;
        CommandPool& m_transPool;

        std::unordered_map<SGUID, std::unique_ptr<TextureBase>> m_textures;
    };
}

#endif //SHIFT_TEXTURESYSTEM_HPP
