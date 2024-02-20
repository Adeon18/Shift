#include "Images.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace sft::gfx {
    TextureBase::TextureBase(const sft::gfx::Device &device,
                     uint32_t width,
                     uint32_t height,
                     uint32_t depth,
                     uint32_t mips,
                     uint32_t levels,
                     VkFormat format,
                     VkImageUsageFlags usage,
                     sft::gfx::TextureDim dim):
     m_device{device}, m_width{width}, m_height{height}, m_mipCount{mips}, m_levels{levels}, m_format{format}, m_dim{dim}
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        switch (m_dim) {
            case TextureDim::T_1D:
                imageInfo.imageType = VK_IMAGE_TYPE_1D;
                break;
            case TextureDim::T_2D:
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                break;
            case TextureDim::T_3D:
                imageInfo.imageType = VK_IMAGE_TYPE_3D;
                break;
        }

        imageInfo.extent.width = m_width;
        imageInfo.extent.height = m_height;
        // Auto defaulr to 1 depth for not 3D textures
        imageInfo.extent.depth = (m_dim == TextureDim::T_3D) ? depth: 1;
        imageInfo.mipLevels = m_mipCount;
        imageInfo.arrayLayers = m_levels;
        imageInfo.format = m_format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        allocCreateInfo.priority = 1.0f;

        vmaCreateImage(m_device.GetAllocator(), &imageInfo, &allocCreateInfo, &m_image, &m_allocation, &m_allocationInfo);

        // TODO: For now does not support cubemaps
        VkImageViewType viewType;
        switch (m_dim) {
            case TextureDim::T_1D:
                viewType = VK_IMAGE_VIEW_TYPE_1D;
                break;
            case TextureDim::T_2D:
                viewType = VK_IMAGE_VIEW_TYPE_2D;
                break;
            case TextureDim::T_3D:
                viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
        }

        //! TODO: handle for depth
        VkImageSubresourceRange sRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = m_mipCount,
            .baseArrayLayer = 0,
            .layerCount = m_levels
        };

        m_imageView = m_device.CreateImageView(info::CreateImageViewInfo(m_image, viewType, format, sRange));
    }

    bool TextureBase::CreateSampler(VkSamplerCreateInfo info) {
        m_sampler = m_device.CreateImageSampler(info);
        return m_sampler != VK_NULL_HANDLE;
    }

    TextureBase::~TextureBase() {
        m_device.DestroyImageSampler(m_sampler);
        m_device.DestroyImageView(m_imageView);
        vmaDestroyImage(m_device.GetAllocator(), m_image, m_allocation);
    }
} // sft::gfx