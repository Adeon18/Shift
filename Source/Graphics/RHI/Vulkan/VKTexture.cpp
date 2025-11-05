#include "VKTexture.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    void Texture::Init(const Device *device, const TextureDescriptor &textureDesc) {
        m_device = device;
        m_textureDesc = textureDesc;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = Util::ShiftToVKTextureType(textureDesc.textureType);

        imageInfo.extent.width = m_textureDesc.width;
        imageInfo.extent.height = m_textureDesc.height;
        imageInfo.extent.depth = m_textureDesc.depth;
        imageInfo.mipLevels = m_textureDesc.mips;
        imageInfo.arrayLayers = m_textureDesc.levels;
        imageInfo.format = Util::ShiftToVKTextureFormat(m_textureDesc.format);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = Util::ShiftToVKResourceLayout(textureDesc.resourceLayout);
        imageInfo.usage = Util::ShiftToVKTextureUsageFlags(textureDesc.usageFlags);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        if ( VkCheck(vmaCreateImage(m_device->GetAllocator(), &imageInfo, &allocCreateInfo, &m_image, &m_allocation, &m_allocationInfo)) ) {
            Log(Warning, "Failed to allocate VkImage!");
            valid = false;
            return;
        }

        // TODO: [FEATURE]: For now does not support cubemaps
        VkImageViewType viewType = Util::ShiftToVKTextureViewType(m_textureDesc.textureViewType) ;
        VkImageAspectFlags textureType = Util::ShiftToVKTextureAspect(m_textureDesc.textureAspect);

        VkImageSubresourceRange sRange{
            .aspectMask = textureType,
            .baseMipLevel = 0,
            .levelCount = m_textureDesc.mips,
            .baseArrayLayer = 0,
            .layerCount = m_textureDesc.levels
        };

        m_imageView = m_device->CreateImageView(Util::CreateImageViewInfo(m_image, viewType, Util::ShiftToVKTextureFormat(m_textureDesc.format), sRange));

        valid = m_imageView != VK_NULL_HANDLE;
    }

    void Texture::Destroy() {
        m_device->DestroyImageView(m_imageView);
        vmaDestroyImage(m_device->GetAllocator(), m_image, m_allocation);
    }
} // Shift::VK