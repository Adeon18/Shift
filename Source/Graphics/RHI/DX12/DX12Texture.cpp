#include "DX12Texture.hpp"

#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    Texture::Texture(const Device* device, const TextureDescriptor& textureDesc) { SRHI_NOT_IMPLEMENTED(); }

    bool Texture::IsValid() const { SRHI_NOT_IMPLEMENTED(); }
    uint32_t Texture::GetWidth() const { SRHI_NOT_IMPLEMENTED(); }
    uint32_t Texture::GetHeight() const { SRHI_NOT_IMPLEMENTED(); }
    uint32_t Texture::GetDepth() const { SRHI_NOT_IMPLEMENTED(); }
    uint32_t Texture::GetMipCount() const { SRHI_NOT_IMPLEMENTED(); }
    uint32_t Texture::GetLevels() const { SRHI_NOT_IMPLEMENTED(); }
    ETextureFormat Texture::GetFormat() const { SRHI_NOT_IMPLEMENTED(); }
    ETextureType Texture::GetType() const { SRHI_NOT_IMPLEMENTED(); }
    ETextureAspect Texture::GetAspect() const { SRHI_NOT_IMPLEMENTED(); }
    ETextureUsageFlags Texture::GetUsageFlags() const { SRHI_NOT_IMPLEMENTED(); }

    TextureSyncState& Texture::GetSyncStateMutable() { SRHI_NOT_IMPLEMENTED(); }
    const TextureSyncState& Texture::GetSyncState() const { SRHI_NOT_IMPLEMENTED(); }

    Texture::~Texture() = default;
} // Shift::DX12
