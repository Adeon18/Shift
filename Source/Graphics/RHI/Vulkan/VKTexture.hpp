#ifndef SHIFT_VKTEXTURE_HPP
#define SHIFT_VKTEXTURE_HPP

#include "VKDevice.hpp"

#include "Graphics/RHI/Common/Texture.hpp"

namespace Shift::VK {
    //! A RAII Wrapper for texture creation/destriction logic, it not mean to be used raw as has a ton of configs
    //! Meant to be used as a base class
    class Texture {
        friend VK::ResourceSet;
    public:
        Texture() = default;

        void Init(const Device* device, const TextureDescriptor& textureDesc);

        [[nodiscard]] bool IsValid() const { return valid; }

        //! TODO [FIX] make these VK_ and private!
        [[nodiscard]] VkImage GetImage() const { return m_image; }
        [[nodiscard]] VkImageView GetView() const { return m_imageView; }
        [[nodiscard]] VmaAllocation GetAlloc() const { return m_allocation; }
        [[nodiscard]] VmaAllocationInfo GetAllocInfo() const { return m_allocationInfo; }

        [[nodiscard]] VkPipelineStageFlags VK_GetStageFlags() const { return m_stageFlags; }
        void VK_SetStageFlags(VkPipelineStageFlags stageFlags) const { m_stageFlags = stageFlags; }

        [[nodiscard]] uint32_t GetWidth() const { return m_textureDesc.width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_textureDesc.height; }
        [[nodiscard]] uint32_t GetDepth() const { return m_textureDesc.height; }
        [[nodiscard]] uint32_t GetMipCount() const { return m_textureDesc.mips; }
        [[nodiscard]] uint32_t GetLevels() const { return m_textureDesc.levels; }
        [[nodiscard]] ETextureFormat GetFormat() const { return m_textureDesc.format; }
        [[nodiscard]] ETextureType GetType() const { return m_textureDesc.textureType; }
        [[nodiscard]] ETextureAspect GetAspect() const { return m_textureDesc.textureAspect; }
        [[nodiscard]] ETextureUsageFlags GetUsageFlags() const { return m_textureDesc.usageFlags; }

        void SetResourceLayout(EResourceLayout layout) const { m_textureDesc.resourceLayout = layout; }
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

        //! These are just cached layouts, really, they are only used to keep track of the resource state to make transtion
        //! functions cleaner
        mutable VkPipelineStageFlags m_stageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        bool valid = false;

        TextureDescriptor m_textureDesc{};
    };

    ASSERT_INTERFACE(ITexture, Texture);
} // Shift::VK

#endif //SHIFT_VKTEXTURE_HPP
