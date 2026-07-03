#ifndef SHIFT_VKTEXTURE_HPP
#define SHIFT_VKTEXTURE_HPP

#include "VKDevice.hpp"

#include "Graphics/RHI/Common/Texture.hpp"

namespace Shift::VK {
    //! A RAII Wrapper for texture creation/destriction logic, it not mean to be used raw as has a ton of configs
    //! Meant to be used as a base class
    class Texture {
        friend VK::ResourceSet;
        friend VK::ImGuiBackend;
        friend VK::CommandBuffer;
    public:
        //! Allocating constructor: creates VkImage via VMA + VkImageView. Owns both.
        Texture(const Device* device, const TextureDescriptor& textureDesc);
        //! Non-owning constructor: wraps an externally-owned VkImage (e.g. swapchain back buffer).
        //! Creates and owns a VkImageView, but does NOT destroy the VkImage.
        Texture(const Device* device, VkImage externalImage, VkImageViewType viewType, const TextureDescriptor& textureDesc);
        Texture(const Texture&)=delete;
        Texture& operator=(const Texture&)=delete;

        [[nodiscard]] bool IsValid() const { return valid; }

        [[nodiscard]] uint32_t GetWidth() const { return m_textureDesc.width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_textureDesc.height; }
        [[nodiscard]] uint32_t GetDepth() const { return m_textureDesc.depth; }
        [[nodiscard]] uint32_t GetMipCount() const { return m_textureDesc.mips; }
        [[nodiscard]] uint32_t GetLevels() const { return m_textureDesc.levels; }
        [[nodiscard]] ETextureFormat GetFormat() const { return m_textureDesc.format; }
        [[nodiscard]] ETextureType GetType() const { return m_textureDesc.textureType; }
        [[nodiscard]] ETextureAspect GetAspect() const { return m_textureDesc.textureAspect; }
        [[nodiscard]] ETextureUsageFlags GetUsageFlags() const { return m_textureDesc.usageFlags; }

        void SetResourceLayout(EResourceLayout layout) const { m_textureDesc.resourceLayout = layout; }
        [[nodiscard]] EResourceLayout GetResourceLayout() const { return m_textureDesc.resourceLayout; }

        ~Texture();
    private:
        //! API SPECIFIC, backend-only (friended). DO NOT USE OUTSIDE THE VK BACKEND.
        [[nodiscard]] VkImage VK_GetImage() const { return m_image; }
        [[nodiscard]] VkImageView VK_GetView() const { return m_imageView; }
        [[nodiscard]] VkPipelineStageFlags2 VK_GetStageFlags() const { return m_stageFlags; }
        void VK_SetStageFlags(VkPipelineStageFlags2 stageFlags) const { m_stageFlags = stageFlags; }

        //! TODO
        void GenerateMips();

        const Device* m_device = nullptr;

        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocationInfo{};
        bool m_ownsImage = true;

        //! These are just cached layouts, really, they are only used to keep track of the resource state to make transtion
        //! functions cleaner
        mutable VkPipelineStageFlags2 m_stageFlags = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;

        bool valid = false;

        TextureDescriptor m_textureDesc{};
    };

    ASSERT_INTERFACE(ITexture, Texture);
} // Shift::VK

#endif //SHIFT_VKTEXTURE_HPP
