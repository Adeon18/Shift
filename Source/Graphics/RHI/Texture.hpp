//
// Created by otrush on 10/1/2024.
//

#ifndef SHIFT_TEXTURE_HPP
#define SHIFT_TEXTURE_HPP

#include <concepts>
#include <type_traits>
#include <string>

#include "Base.hpp"
#include "Types.hpp"
#include "TextureFormat.hpp"

namespace Shift {
    //! Shift Texture Type
    //! Note: 1:1 with Vulkan
    enum class ETextureType {
        Texture1D = 0,
        Texture2D = 1,
        Texture3D = 2
    };

    //! Shift Texture View Type
    //! Note: 1:1 with Vulkan
    enum class ETextureViewType {
        View1D = 0,
        View2D = 1,
        View3D = 2,
        ViewCubemap = 3,
        View1DArray = 4,
        View2DArray = 5,
        ViewCubemapArray = 6
    };

    //! Shift Texture Usage Flags
    //! Note: 1:1 with Vulkan
    enum class ETextureUsageFlags {
        None = 0,
        TransferSrc = 1 << 0,
        TransferDst = 1 << 1,
        Sampled = 1 << 2,
        Storage = 1 << 3,
        ColorAttachment = 1 << 4,
        DepthStencilAttachment = 1 << 5,
    };
    DEFINE_ENUM_CLASS_BITWISE_OPERATORS(ETextureUsageFlags)

    //! Shift Texture Aspect
    //! Note: 1:1 with Vulkan, skipping some
    enum class ETextureAspect {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        Metadata = 1 << 3
    };
    DEFINE_ENUM_CLASS_BITWISE_OPERATORS(ETextureAspect)

    //! 1:1 with Vulkan
    enum class ETilingMode {
        Optimal,
        Linear
    };

    //! 1:1 with Vulkan
    struct TextureSubresourceRange {
        ETextureAspect aspect;
        uint32_t baseMipLevel;
        uint32_t levelCount;
        uint32_t baseArrayLayer;
        uint32_t layerCount;
    };

    //! The texture description structure
    //! For now there is no texture view as I believe it should be platform-specific
    struct TextureDescriptor {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 1;
        uint32_t mips = 1;
        uint32_t levels = 1;

        ETextureFormat format = ETextureFormat::R8G8B8A8_SRGB;
        ETextureUsageFlags usageFlags = ETextureUsageFlags::Sampled;
        ETextureType textureType = ETextureType::Texture2D;
        ETextureViewType textureViewType = ETextureViewType::View2D;
        ETextureAspect textureAspect = ETextureAspect::Color;
        EResourceLayout resourceLayout = EResourceLayout::Undefined;
        const char* name = "EMPTY";

        static TextureDescriptor CreateTexture2DDesc(
            uint32_t width,
            uint32_t height,
            const char *name,
            ETextureFormat format,
            uint32_t mipCount,
            ETextureUsageFlags usageFlags,
            ETextureAspect textureAspect,
            EResourceLayout resourceLayout = EResourceLayout::Undefined,
            bool isCubemap = false
        ) {
            return {
                .width = width,
                .height = height,
                .depth = 1,
                .mips = mipCount,
                .levels = 1,
                .format = format,
                .usageFlags = usageFlags,
                .textureType = ETextureType::Texture2D,
                .textureViewType = ETextureViewType::View2D,
                .textureAspect = textureAspect,
                .resourceLayout = resourceLayout,
                .name = name
            };
        }

        static TextureDescriptor CreateDepthTextureDesc(
            uint32_t width,
            uint32_t height,
            const char *name,
            ETextureFormat format
        ) {
            return CreateTexture2DDesc(width, height, name, format, 1, ETextureUsageFlags::DepthStencilAttachment, ETextureAspect::Depth);
        }

        static TextureDescriptor CreateSampledRenderTarget2DDesc(
                uint32_t width,
                uint32_t height,
                const char *name,
                ETextureFormat format,
                ETextureUsageFlags usage,
                ETextureAspect textureAspect,
                uint32_t mipCount = 1
        ) {
            return CreateTexture2DDesc(width, height, name, format, mipCount, usage | ETextureUsageFlags::Sampled, textureAspect);
        }

        static TextureDescriptor CreateColorRenderTarget2DDesc(
                uint32_t width,
                uint32_t height,
                const char *name,
                ETextureFormat format,
                ETextureUsageFlags usage,
                uint32_t mipCount = 1
        ) {
            return CreateTexture2DDesc(width, height, name, format, mipCount, usage | ETextureUsageFlags::Sampled, ETextureAspect::Color);
        }

        static TextureDescriptor CreateDepthRenderTarget2D(
                uint32_t width,
                uint32_t height,
                const char *name,
                ETextureFormat format,
                ETextureUsageFlags usage,
                uint32_t mipCount = 1
        ) {
            return CreateTexture2DDesc(width, height, name, format, mipCount, usage | ETextureUsageFlags::Sampled, ETextureAspect::Depth);
        }
    };

    //! A concept that acts as an interface for all Graphics API Buffer classes.
    //! Yes, this could just be a base non-virtual class but such approach can be only followed with the Texture in
    //! Shift RHI design so I aint mixing styles:D
    //! \tparam Texture
    template<typename Texture>
    concept ITexture =
        std::is_default_constructible_v<Texture> &&
        std::is_trivially_destructible_v<Texture> &&
    requires(Texture InputTexture, const TextureDescriptor& TextureDesc, const Device* DevicePtr) {
        { InputTexture.Init(DevicePtr, TextureDesc) } -> std::same_as<bool>;
        { InputTexture.Destroy() } -> std::same_as<void>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetWidth() } -> std::same_as<uint32_t>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetHeight() } -> std::same_as<uint32_t>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetDepth() } -> std::same_as<uint32_t>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetMipCount() } -> std::same_as<uint32_t>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetLevels() } -> std::same_as<uint32_t>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetFormat() } -> std::same_as<ETextureFormat>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetType() } -> std::same_as<ETextureType>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetAspect() } -> std::same_as<ETextureAspect>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetUsageFlags() } -> std::same_as<ETextureUsageFlags>;
        { CONCEPT_CONST_VAR(Texture, InputTexture).GetView() };
    };
} // Shift

#endif //SHIFT_TEXTURE_HPP
