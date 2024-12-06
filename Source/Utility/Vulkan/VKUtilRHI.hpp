//
// Created by otrush on 11/7/2024.
//

#ifndef SHIFT_UTILRHI_HPP
#define SHIFT_UTILRHI_HPP

#include <vulkan/vulkan.h>

#include "Graphics/RHI/Texture.hpp"
#include "Graphics/RHI/TextureFormat.hpp"
#include "Graphics/RHI/Sampler.hpp"
#include "Graphics/RHI/Pipeline.hpp"
#include "Graphics/RHI/RenderPass.hpp"

namespace Shift::VK::Util {
    //! Create a VkImageType from an ETextureType
    //! \param type The texture type enum value to convert
    //! \return The corresponding VkImageType
    VkImageType ShiftToVKTextureType(ETextureType type);

    //! Create a VkImageViewType from an ETextureViewType
    //! \param viewType The texture view type enum value to convert
    //! \return The corresponding VkImageViewType
    VkImageViewType ShiftToVKTextureViewType(ETextureViewType viewType);

    //! Create VkImageUsageFlags from ETextureUsageFlags
    //! \param flags The texture usage flags enum value to convert
    //! \return The corresponding VkImageUsageFlags
    VkImageUsageFlags ShiftToVKTextureUsageFlags(ETextureUsageFlags flags);

    //! Create VkImageAspectFlags from ETextureAspect
    //! \param aspect The texture aspect enum value to convert
    //! \return The corresponding VkImageAspectFlags
    VkImageAspectFlags ShiftToVKTextureAspect(ETextureAspect aspect);

    //! Convert a TextureSubresourceRange to a VkImageSubresourceRange
    //! \param range The texture subresource range to convert
    //! \return The corresponding VkImageSubresourceRange
    VkImageSubresourceRange ShiftToVKSubresourceRange(const TextureSubresourceRange& range);

    //! Create a VkImageLayout from an EResourceLayout
    //! \param layout The resource layout enum value to convert
    //! \return The corresponding VkImageLayout
    VkImageLayout ShiftToVKResourceLayout(EResourceLayout layout);

    //! Create a VkFormat from an ETextureFormat
    //! \param format The texture format enum value to convert
    //! \return The corresponding VkFormat
    VkFormat ShiftToVKTextureFormat(ETextureFormat format);

    //! Create a VkFormat from an EVertexAttributeFormat, since Shift has different enums for both
    //! \param format The vertex format enum value to convert
    //! \return The corresponding VkFormat
    VkFormat ShiftToVKVertexFormat(EVertexAttributeFormat format);

    //! Create a VkFilter from an EFilterMode
    //! \param mode The filter mode enum value to convert
    //! \return The corresponding VkFilter
    VkFilter ShiftToVKFilterMode(EFilterMode mode);

    //! Create a VkSamplerAddressMode from an ESamplerAddressMode
    //! \param addressMode The sampler address mode enum value to convert
    //! \return The corresponding VkSamplerAddressMode
    VkSamplerAddressMode ShiftToVKSamplerAddressMode(ESamplerAddressMode addressMode);

    //! Create a VkSamplerMipmapMode from an EMipMapMode
    //! \param mipMapMode The mipmap mode enum value to convert
    //! \return The corresponding VkSamplerMipmapMode
    VkSamplerMipmapMode ShiftToVKMipMapMode(EMipMapMode mipMapMode);

    //! Create a VkAttachmentLoadOp from an EAttachmentLoadOperation
    //! \param operation The attachment load operation enum value to convert
    //! \return The corresponding VkAttachmentLoadOp
    VkAttachmentLoadOp ShiftToVKAttachmentLoadOperation(EAttachmentLoadOperation operation);

    //! Create a VkAttachmentStoreOp from an EAttachmentStoreOperation
    //! \param operation The attachment store operation enum value to convert
    //! \return The corresponding VkAttachmentStoreOp
    VkAttachmentStoreOp ShiftToVKAttachmentStoreOperation(EAttachmentStoreOperation operation);

    //! Create a VkStencilOp from an EStencilOp
    //! \param op The stencil operation enum value to convert
    //! \return The corresponding VkStencilOp
    VkStencilOp ShiftToVKStencilOp(EStencilOp op);

    //! Create a VkPolygonMode from an EPolygonMode
    //! \param mode The polygon mode enum value to convert
    //! \return The corresponding VkPolygonMode
    VkPolygonMode ShiftToVKPolygonMode(EPolygonMode mode);

    //! Create a VkCullModeFlags from an ECullMode
    //! \param mode The cull mode enum value to convert
    //! \return The corresponding VkCullModeFlags
    VkCullModeFlagBits ShiftToVKCullMode(ECullMode mode);

    //! Create a VkFrontFace from an EWindingOrder
    //! \param order The winding order enum value to convert
    //! \return The corresponding VkFrontFace
    VkFrontFace ShiftToVKWindingOrder(EWindingOrder order);

    //! Create a VkColorComponentFlags from an EColorWriteMask
    //! \param mask The color write mask enum value to convert
    //! \return The corresponding VkColorComponentFlags
    VkColorComponentFlags ShiftToVKColorWriteMask(EColorWriteMask mask);

    //! Create a VkBlendFactor from an EBlendFactor
    //! \param factor The blend factor enum value to convert
    //! \return The corresponding VkBlendFactor
    VkBlendFactor ShiftToVKBlendFactor(EBlendFactor factor);

    //! Create a VkBlendOp from an EBlendOperation
    //! \param operation The blend operation enum value to convert
    //! \return The corresponding VkBlendOp
    VkBlendOp ShiftToVKBlendOperation(EBlendOperation operation);

    //! Create a VkLogicOp from an ELogicalOperation
    //! \param operation Shift logical operation
    //! \return The corresponding VkLogicOp
    VkLogicOp ShiftToVKLogicalOperation(PipelineDescriptor::ColorBlendConfig::ELogicalOperation operation);

    //! Create a VkCompareOp from an ECompareOperation
    //! \param compareOp Shift compare operation
    //! \return The corresponding VkCompareOp
    VkCompareOp ShiftToVKCompareOperation(ECompareOperation compareOp);

    //! Create a VkVertexInputRate from an EVertexInputRate
    //! \param rate The vertex input rate enum value to convert
    //! \return The corresponding VkVertexInputRate
    VkVertexInputRate ShiftToVKVertexInputRate(EVertexInputRate rate);

    //! Create a VkPrimitiveTopology from an EPrimitiveTopology
    //! \param topology The primitive topology enum value to convert
    //! \return The corresponding VkPrimitiveTopology
    VkPrimitiveTopology ShiftToVKPrimitiveTopology(EPrimitiveTopology topology);

    //! Create a VkDescriptorType from an EBindingType
    //! \param bindingType The binding type enum value to convert
    //! \return The corresponding VkDescriptorType
    VkDescriptorType ShiftToVKBindingType(EBindingType bindingType);

    //! Create a VkShaderStageFlags from an EBindingVisibility
    //! \param visibility The binding visibility enum value to convert
    //! \return The corresponding VkShaderStageFlags
    VkShaderStageFlagBits ShiftToVKBindingVisibility(EBindingVisibility visibility);

    //! Create a VkShaderStageFlags from an EShaderType
    //! \param visibility The shader type
    //! \return The corresponding VkShaderStageFlags
    VkShaderStageFlagBits ShiftToVKShaderType(EShaderType type);

    //! Convert a VertexConfig structure to a Vulkan vertex input state create info structure
    //! \param config The vertex configuration descriptor to convert
    //! \return The corresponding VkPipelineVertexInputStateCreateInfo structure
    VkPipelineVertexInputStateCreateInfo ShiftToVKVertexConfig(const PipelineDescriptor::VertexConfig& config);

    //! Convert a RasterizerStateDesc structure to a Vulkan rasterization state create info structure
    //! \param desc The rasterizer state descriptor to convert
    //! \return The corresponding VkPipelineRasterizationStateCreateInfo structure
    VkPipelineRasterizationStateCreateInfo ShiftToVKRasterizerState(const PipelineDescriptor::RasterizerStateDesc& desc);

    //! Convert a MultisampleDesc structure to a Vulkan multisample state create info structure
    //! \param desc The multisample descriptor to convert
    //! \return The corresponding VkPipelineRasterizationStateCreateInfo structure
    VkPipelineMultisampleStateCreateInfo ShiftToVKMultisampleDesc(const PipelineDescriptor::MultisampleDesc& desc);

    //! Convert each color blend attachment to Vulkan versions
    //! \param config The color blend configuration descriptor to pull attachment descriptions from
    //! \param outAttachments A pointer to the vector with converted attachment descs
    void ShiftToVKColorAttachmentConfig(const PipelineDescriptor::ColorBlendConfig& config, std::vector<VkPipelineColorBlendAttachmentState>* outAttachments);

    //! Convert a ColorBlendConfig structure to a Vulkan color blend state create info structure
    //! \param config The color blend configuration descriptor to convert
    //! \param vkAttachments The already converted vulkan blend attachments
    //! \param vkAttachments The already converted vulkan blend attachments
    //! \return The corresponding VkPipelineColorBlendStateCreateInfo structure
    VkPipelineColorBlendStateCreateInfo ShiftToVKColorBlendConfig(const PipelineDescriptor::ColorBlendConfig& config,  const std::vector<VkPipelineColorBlendAttachmentState>& vkAttachments);

    //! Convert a DepthStencilConfig structure to a Vulkan depth stencil state create info structure
    //! \param config The depth stencil configuration descriptor to convert
    //! \return The corresponding VkPipelineColorBlendStateCreateInfo structure
    VkPipelineDepthStencilStateCreateInfo ShiftToVKDepthStencilConfig(const PipelineDescriptor::DepthStencilConfig& config);

    //!-------------------------------------VKTOShift-------------------------------------!//

    //! Create an ETextureType from a VkImageType
    //! \param type The VkImageType value to convert
    //! \return The corresponding ETextureType
    ETextureType VKToShiftTextureType(VkImageType type);

    //! Create an ETextureViewType from a VkImageViewType
    //! \param viewType The VkImageViewType value to convert
    //! \return The corresponding ETextureViewType
    ETextureViewType VKToShiftTextureViewType(VkImageViewType viewType);

    //! Create ETextureUsageFlags from VkImageUsageFlags
    //! \param flags The VkImageUsageFlags value to convert
    //! \return The corresponding ETextureUsageFlags
    ETextureUsageFlags VKToShiftTextureUsageFlags(VkImageUsageFlags flags);

    //! Create ETextureAspect from VkImageAspectFlags
    //! \param aspect The VkImageAspectFlags value to convert
    //! \return The corresponding ETextureAspect
    ETextureAspect VKToShiftTextureAspect(VkImageAspectFlags aspect);

    //! Convert a VkImageSubresourceRange to a TextureSubresourceRange
    //! \param range The VkImageSubresourceRange to convert
    //! \return The corresponding TextureSubresourceRange
    TextureSubresourceRange VKToShiftSubresourceRange(const VkImageSubresourceRange& range);

    //! Create an EResourceLayout from a VkImageLayout
    //! \param layout The VkImageLayout value to convert
    //! \return The corresponding EResourceLayout
    EResourceLayout VKToShiftResourceLayout(VkImageLayout layout);

    //! Create an ETextureFormat from a VkFormat
    //! \param format The VkFormat value to convert
    //! \return The corresponding ETextureFormat
    ETextureFormat VKToShiftTextureFormat(VkFormat format);

    //! Create an EFilterMode from a VkFilter
    //! \param mode The VkFilter value to convert
    //! \return The corresponding EFilterMode
    EFilterMode VKToShiftFilterMode(VkFilter mode);

    //! Create an ESamplerAddressMode from a VkSamplerAddressMode
    //! \param addressMode The VkSamplerAddressMode value to convert
    //! \return The corresponding ESamplerAddressMode
    ESamplerAddressMode VKToShiftSamplerAddressMode(VkSamplerAddressMode addressMode);

    //! Create an EMipMapMode from a VkSamplerMipmapMode
    //! \param mipMapMode The VkSamplerMipmapMode value to convert
    //! \return The corresponding EMipMapMode
    EMipMapMode VKToShiftMipMapMode(VkSamplerMipmapMode mipMapMode);

    //! Create an EAttachmentLoadOperation from a VkAttachmentLoadOp
    //! \param operation The VkAttachmentLoadOp value to convert
    //! \return The corresponding EAttachmentLoadOperation
    EAttachmentLoadOperation VKToShiftAttachmentLoadOperation(VkAttachmentLoadOp operation);

    //! Create an EAttachmentStoreOperation from a VkAttachmentStoreOp
    //! \param operation The VkAttachmentStoreOp value to convert
    //! \return The corresponding EAttachmentStoreOperation
    EAttachmentStoreOperation VKToShiftAttachmentStoreOperation(VkAttachmentStoreOp operation);

    //! Create an EStencilOp from a VkStencilOp
    //! \param op The VkStencilOp value to convert
    //! \return The corresponding EStencilOp
    EStencilOp VKToShiftStencilOp(VkStencilOp op);

    //! Create an EPolygonMode from a VkPolygonMode
    //! \param mode The VkPolygonMode value to convert
    //! \return The corresponding EPolygonMode
    EPolygonMode VKToShiftPolygonMode(VkPolygonMode mode);

    //! Create an ECullMode from a VkCullModeFlags
    //! \param mode The VkCullModeFlags value to convert
    //! \return The corresponding ECullMode
    ECullMode VKToShiftCullMode(VkCullModeFlagBits mode);

    //! Create an EWindingOrder from a VkFrontFace
    //! \param order The VkFrontFace value to convert
    //! \return The corresponding EWindingOrder
    EWindingOrder VKToShiftWindingOrder(VkFrontFace order);

    //! Create an EColorWriteMask from a VkColorComponentFlags
    //! \param mask The VkColorComponentFlags value to convert
    //! \return The corresponding EColorWriteMask
    EColorWriteMask VKToShiftColorWriteMask(VkColorComponentFlags mask);

    //! Create an EBlendFactor from a VkBlendFactor
    //! \param factor The VkBlendFactor value to convert
    //! \return The corresponding EBlendFactor
    EBlendFactor VKToShiftBlendFactor(VkBlendFactor factor);

    //! Create an EBlendOperation from a VkBlendOp
    //! \param operation The VkBlendOp value to convert
    //! \return The corresponding EBlendOperation
    EBlendOperation VKToShiftBlendOperation(VkBlendOp operation);

    //! Create an EVertexInputRate from a VkVertexInputRate
    //! \param rate The VkVertexInputRate value to convert
    //! \return The corresponding EVertexInputRate
    EVertexInputRate VKToShiftVertexInputRate(VkVertexInputRate rate);

    //! Create an EPrimitiveTopology from a VkPrimitiveTopology
    //! \param topology The VkPrimitiveTopology value to convert
    //! \return The corresponding EPrimitiveTopology
    EPrimitiveTopology VKToShiftPrimitiveTopology(VkPrimitiveTopology topology);

    //! Create an EBindingType from a VkDescriptorType
    //! \param bindingType The VkDescriptorType value to convert
    //! \return The corresponding EBindingType
    EBindingType VKToShiftBindingType(VkDescriptorType bindingType);

    //! Create an EBindingVisibility from a VkShaderStageFlags TODO
    //! \param visibility The VkShaderStageFlags value to convert
    //! \return The corresponding EBindingVisibility
    EBindingVisibility VKToShiftBindingVisibility(VkShaderStageFlagBits visibility);
}

#endif //SHIFT_UTILRHI_HPP
