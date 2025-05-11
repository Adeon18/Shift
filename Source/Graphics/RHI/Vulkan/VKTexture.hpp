#ifndef SHIFT_VKTEXTURE_HPP
#define SHIFT_VKTEXTURE_HPP

#include "VKDevice.hpp"

#include "Graphics/RHI/Texture.hpp"

namespace Shift::VK {
    //! A RAII Wrapper for texture creation/destriction logic, it not mean to be used raw as has a ton of configs
    //! Meant to be used as a base class
    class Texture {
        friend VK::ResourceSet;
    public:
        Texture() = default;
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        [[nodiscard]] bool Init(const Device* device, const TextureDescriptor& textureDesc);

        [[nodiscard]] VkImage GetImage() const { return m_image; }
        [[nodiscard]] VkImageView GetView() const { return m_imageView; }
        [[nodiscard]] VmaAllocation GetAlloc() const { return m_allocation; }
        [[nodiscard]] VmaAllocationInfo GetAllocInfo() const { return m_allocationInfo; }

        [[nodiscard]] uint32_t GetWidth() const { return m_textureDesc.width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_textureDesc.height; }
        [[nodiscard]] uint32_t GetDepth() const { return m_textureDesc.height; }
        [[nodiscard]] uint32_t GetMipCount() const { return m_textureDesc.mips; }
        [[nodiscard]] uint32_t GetLevels() const { return m_textureDesc.levels; }
        [[nodiscard]] ETextureFormat GetFormat() const { return m_textureDesc.format; }
        [[nodiscard]] ETextureType GetType() const { return m_textureDesc.textureType; }
        [[nodiscard]] ETextureAspect GetAspect() const { return m_textureDesc.textureAspect; }
        [[nodiscard]] ETextureUsageFlags GetUsageFlags() const { return m_textureDesc.usageFlags; }
        [[nodiscard]] EResourceLayout GetResourceLayout() const { return m_textureDesc.resourceLayout; }

        void Destroy();
        ~Texture() = default;
    private:
        //! TODO
        void GenerateMips();

        const Device* m_device = nullptr;

        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocationInfo{};

        TextureDescriptor m_textureDesc{};
    };

    ASSERT_INTERFACE(ITexture, Texture);
} // Shift::VK

#endif //SHIFT_VKTEXTURE_HPP
