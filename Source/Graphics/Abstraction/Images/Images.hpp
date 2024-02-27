#ifndef SHIFT_IMAGES_HPP
#define SHIFT_IMAGES_HPP

#include "Graphics/Abstraction/Device/Device.hpp"

namespace shift::gfx {
    enum class TextureDim {
        T_1D,
        T_2D,
        T_3D,
    };

    enum class TextureType {
        Color,
        Depth,
    };


    //! A RAII Wrapper for texture creation/destriction logic, it not mean to be used raw as has a ton of configs
    //! Meant to be used as a base class
    //! Constructor just creates image, view and allocation, everything else should be explicit
    //! Is NOT virtusl for now, MAYBE TODO
    class TextureBase {
    public:
        TextureBase(
                const Device& device,
                uint32_t width,
                uint32_t height,
                uint32_t depth,
                uint32_t mips,
                uint32_t levels,
                VkFormat format,
                VkImageUsageFlags usage,
                TextureDim dim,
                TextureType texType = TextureType::Color
            );

        bool CreateSampler(VkSamplerCreateInfo info);

        [[nodiscard]] VkImage GetImage() const { return m_image; }
        [[nodiscard]] VkImageView GetView() const { return m_imageView; }
        [[nodiscard]] VmaAllocation GetAlloc() const { return m_allocation; }
        [[nodiscard]] VmaAllocationInfo GetAllocInfo() const { return m_allocationInfo; }
        [[nodiscard]] VkSampler GetSampler() const { return m_sampler; }

        [[nodiscard]] uint32_t GetWidth() const { return m_width; }
        [[nodiscard]] uint32_t GetHeight() const { return m_height; }
        [[nodiscard]] uint32_t GetMipCount() const { return m_mipCount; }
        [[nodiscard]] uint32_t GetLevels() const { return m_levels; }
        [[nodiscard]] VkFormat GetFormat() const { return m_format; }
        [[nodiscard]] TextureDim GetDimension() const { return m_dim; }

        ~TextureBase();

        TextureBase() = delete;
        TextureBase(const TextureBase&) = delete;
        TextureBase& operator=(const TextureBase&) = delete;
    private:
        //! TODO
        void GenerateMips();

        const Device& m_device;

        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
        VmaAllocationInfo m_allocationInfo;

        VkSampler m_sampler = VK_NULL_HANDLE;

        uint32_t m_width;
        uint32_t m_height;

        uint32_t m_mipCount;
        uint32_t m_levels;

        TextureDim m_dim;
        TextureType m_texType;
        VkFormat m_format;
    };

    class Texture2D: public TextureBase {
    public:
        Texture2D(
                const Device& device,
                uint32_t width,
                uint32_t height,
                VkFormat format,
                VkImageUsageFlags usage,
                uint32_t mips = 1
        ) : TextureBase(device, width, height, 1, mips, 1, format, usage, TextureDim::T_2D)
        {

        }
    };

    class DepthTexture: public TextureBase {
    public:
        DepthTexture(
                const Device& device,
                uint32_t width,
                uint32_t height,
                VkFormat format,
                uint32_t mips = 1
        ) : TextureBase(device, width, height, 1, 1, 1, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, TextureDim::T_2D, TextureType::Depth)
        {

        }
    };
} // shift::gfx

#endif //SHIFT_IMAGES_HPP
