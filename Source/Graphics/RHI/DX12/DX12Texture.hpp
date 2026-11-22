#ifndef SHIFT_DX12TEXTURE_HPP
#define SHIFT_DX12TEXTURE_HPP

#include <cstdint>

#include "DX12Device.hpp"
#include "../Common/Texture.hpp"

namespace Shift::DX12 {
    class Texture {
    public:
        Texture(const Device* device, const TextureDescriptor& textureDesc);

        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] uint32_t GetWidth() const;
        [[nodiscard]] uint32_t GetHeight() const;
        [[nodiscard]] uint32_t GetDepth() const;
        [[nodiscard]] uint32_t GetMipCount() const;
        [[nodiscard]] uint32_t GetLevels() const;
        [[nodiscard]] ETextureFormat GetFormat() const;
        [[nodiscard]] ETextureType GetType() const;
        [[nodiscard]] ETextureAspect GetAspect() const;
        [[nodiscard]] ETextureUsageFlags GetUsageFlags() const;

        [[nodiscard]] TextureSyncState& GetSyncStateMutable();
        [[nodiscard]] const TextureSyncState& GetSyncState() const;

        ~Texture();
    };

    ASSERT_INTERFACE(ITexture, Texture);
} // Shift::DX12

#endif //SHIFT_DX12TEXTURE_HPP
