//
// Created by otrush on 11/7/2024.
//

#include <vector>

#include "VKUtilRHI.hpp"

namespace Shift::VK::Util {
    VkImageType ShiftToVKTextureType(ETextureType type) {
        return static_cast<VkImageType>(type);
    }

    VkImageViewType ShiftToVKTextureViewType(ETextureViewType viewType) {
        return static_cast<VkImageViewType>(viewType);
    }

    VkImageUsageFlags ShiftToVKTextureUsageFlags(ETextureUsageFlags flags) {
        return static_cast<VkImageUsageFlags>(flags);
    }

    VkImageAspectFlags ShiftToVKTextureAspect(ETextureAspect aspect) {
        return static_cast<VkImageAspectFlags>(aspect);
    }

    VkImageSubresourceRange ShiftToVKSubresourceRange(const TextureSubresourceRange& range) {
        return {
                ShiftToVKTextureAspect(range.aspect), // aspectMask
                range.baseMipLevel,                   // baseMipLevel
                range.levelCount,                     // levelCount
                range.baseArrayLayer,                 // baseArrayLayer
                range.layerCount                      // layerCount
        };
    }

    VkImageLayout ShiftToVKResourceLayout(EResourceLayout layout) {
        return static_cast<VkImageLayout>(layout);
    }

    VkFormat ShiftToVKTextureFormat(ETextureFormat format) {
        return static_cast<VkFormat>(format);
    }

    VkFormat ShiftToVKVertexFormat(EVertexAttributeFormat format) {
        return static_cast<VkFormat>(format);
    }

    VkAttachmentLoadOp ShiftToVKAttachmentLoadOperation(EAttachmentLoadOperation operation) {
        return static_cast<VkAttachmentLoadOp>(operation);
    }

    VkAttachmentStoreOp ShiftToVKAttachmentStoreOperation(EAttachmentStoreOperation operation) {
        return static_cast<VkAttachmentStoreOp>(operation);
    }

    VkStencilOp ShiftToVKStencilOp(EStencilOp op) {
        return static_cast<VkStencilOp>(op);
    }

    VkPolygonMode ShiftToVKPolygonMode(EPolygonMode mode) {
        return static_cast<VkPolygonMode>(mode);
    }

    VkCullModeFlagBits ShiftToVKCullMode(ECullMode mode) {
        return static_cast<VkCullModeFlagBits>(mode);
    }

    VkFrontFace ShiftToVKWindingOrder(EWindingOrder order) {
        return static_cast<VkFrontFace>(order);
    }

    VkColorComponentFlags ShiftToVKColorWriteMask(EColorWriteMask mask) {
        switch(mask) {
            case EColorWriteMask::RGB: return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
            case EColorWriteMask::RGBA: return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            default: return static_cast<VkColorComponentFlags>(mask);
        }
    }

    VkBlendFactor ShiftToVKBlendFactor(EBlendFactor factor) {
        return static_cast<VkBlendFactor>(factor);
    }

    VkBlendOp ShiftToVKBlendOperation(EBlendOperation operation) {
        return static_cast<VkBlendOp>(operation);
    }

    VkLogicOp ShiftToVKLogicalOperation(PipelineDescriptor::ColorBlendConfig::ELogicalOperation operation) {
        return static_cast<VkLogicOp>(operation);
    }

    VkCompareOp ShiftToVKCompareOperation(ECompareOperation compareOp) {
        return static_cast<VkCompareOp>(compareOp);
    }

    VkVertexInputRate ShiftToVKVertexInputRate(EVertexInputRate rate) {
        return static_cast<VkVertexInputRate>(rate);
    }

    VkPrimitiveTopology ShiftToVKPrimitiveTopology(EPrimitiveTopology topology) {
        return static_cast<VkPrimitiveTopology>(topology);
    }

    VkDescriptorType ShiftToVKBindingType(EBindingType bindingType) {
        return static_cast<VkDescriptorType>(bindingType);
    }

    VkShaderStageFlagBits ShiftToVKBindingVisibility(EBindingVisibility visibility) {
        return static_cast<VkShaderStageFlagBits>(visibility);
    }

    VkShaderStageFlagBits ShiftToVKShaderType(EShaderType type) {
        return static_cast<VkShaderStageFlagBits>(type);
    }

    VkPipelineVertexInputStateCreateInfo ShiftToVKVertexConfig(const PipelineDescriptor::VertexConfig& config) {
        VkPipelineVertexInputStateCreateInfo outInfo{};
        outInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        std::vector<VkVertexInputBindingDescription> bindDesc;
        bindDesc.reserve(config.vertexBindings.size());
        for (const auto& bind: config.vertexBindings) {
            bindDesc.emplace_back(bind.binding, bind.stride, ShiftToVKVertexInputRate(bind.inputRate));
        }
        std::vector<VkVertexInputAttributeDescription> attDesc;
        attDesc.reserve(config.attributeDescs.size());
        for (const auto& attr: config.attributeDescs) {
            attDesc.emplace_back(attr.location, attr.binding, ShiftToVKVertexFormat(attr.format), attr.offset);
        }

        outInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindDesc.size());
        outInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attDesc.size());
        outInfo.pVertexBindingDescriptions = bindDesc.data();
        outInfo.pVertexAttributeDescriptions = attDesc.data();

        return outInfo;
    }

    VkPipelineRasterizationStateCreateInfo ShiftToVKRasterizerState(const PipelineDescriptor::RasterizerStateDesc& desc) {
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        // Clamps objects to min/max depth instead of discard
        rasterizer.depthClampEnable = desc.depthClampEnable;
        // If enabled, the fragments will mever pass on to the framebuffer
        rasterizer.rasterizerDiscardEnable = desc.rasterizerDiscardEnable;
        rasterizer.polygonMode = ShiftToVKPolygonMode(desc.polygoneMode);
        rasterizer.lineWidth = desc.lineWidth;
        rasterizer.cullMode = ShiftToVKCullMode(desc.cullMode);
        rasterizer.frontFace = ShiftToVKWindingOrder(desc.windingOrder);
        rasterizer.depthBiasEnable = desc.depthBias.enable;
        rasterizer.depthBiasConstantFactor = desc.depthBias.constantFactor;
        rasterizer.depthBiasClamp = desc.depthBias.clamp;
        rasterizer.depthBiasSlopeFactor = desc.depthBias.slopeFactor;

        return rasterizer;
    }

    //! TODO [FEATURE] no multisampling for now
    VkPipelineMultisampleStateCreateInfo ShiftToVKMultisampleDesc(const PipelineDescriptor::MultisampleDesc& desc) {
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        return multisampling;
    }

    void ShiftToVKColorAttachmentConfig(const PipelineDescriptor::ColorBlendConfig& config, std::vector<VkPipelineColorBlendAttachmentState>* outAttachments) {
        for (auto& att: config.attachments) {
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = ShiftToVKColorWriteMask(att.colorWriteMask);
            colorBlendAttachment.blendEnable = att.blendEnabled;
            //! Optional
            colorBlendAttachment.srcColorBlendFactor = ShiftToVKBlendFactor(att.sourceColorBlendFactor);
            colorBlendAttachment.dstColorBlendFactor = ShiftToVKBlendFactor(att.destinationColorBlendFactor);
            colorBlendAttachment.colorBlendOp = ShiftToVKBlendOperation(att.colorBlendOperation);
            colorBlendAttachment.srcAlphaBlendFactor = ShiftToVKBlendFactor(att.sourceAlphaBlendFactor);
            colorBlendAttachment.dstAlphaBlendFactor = ShiftToVKBlendFactor(att.destinationAlphaBlendFactor);
            colorBlendAttachment.alphaBlendOp = ShiftToVKBlendOperation(att.alphaBlendOperation);

            outAttachments->push_back(std::move(colorBlendAttachment));
        }
    }

    VkPipelineColorBlendStateCreateInfo ShiftToVKColorBlendConfig(const PipelineDescriptor::ColorBlendConfig& config, const std::vector<VkPipelineColorBlendAttachmentState>& vkAttachments) {
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = vkAttachments.size();
        colorBlending.pAttachments = vkAttachments.data();
        colorBlending.logicOpEnable = config.logicalOpEnabled;
        // Optional
        colorBlending.logicOp = ShiftToVKLogicalOperation(config.logicalOperation);
        colorBlending.blendConstants[0] = config.blendConstants[0];
        colorBlending.blendConstants[1] = config.blendConstants[1];
        colorBlending.blendConstants[2] = config.blendConstants[2];
        colorBlending.blendConstants[3] = config.blendConstants[3];

        return colorBlending;
    }

    VkPipelineDepthStencilStateCreateInfo ShiftToVKDepthStencilConfig(const PipelineDescriptor::DepthStencilConfig& config) {
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = config.depthTestEnabled;
        depthStencil.depthWriteEnable = config.depthWriteEnabled;
        depthStencil.depthCompareOp = ShiftToVKCompareOperation(config.depthFunction);
        // Optional
        //! TODO [FEATURE] add support for stencil and depth bounds
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        return depthStencil;
    }

    //!-------------------------------------VKTOShift-------------------------------------!//

    ETextureType VKToShiftTextureType(VkImageType type) {
        return static_cast<ETextureType>(type);
    }

    ETextureViewType VKToShiftTextureViewType(VkImageViewType viewType) {
        return static_cast<ETextureViewType>(viewType);
    }

    ETextureUsageFlags VKToShiftTextureUsageFlags(VkImageUsageFlags flags) {
        return static_cast<ETextureUsageFlags>(flags);
    }

    ETextureAspect VKToShiftTextureAspect(VkImageAspectFlags aspect) {
        return static_cast<ETextureAspect>(aspect);
    }

    TextureSubresourceRange VKToShiftSubresourceRange(const VkImageSubresourceRange& range) {
        return {
                VKToShiftTextureAspect(range.aspectMask), // aspect
                range.baseMipLevel,                       // baseMipLevel
                range.levelCount,                         // levelCount
                range.baseArrayLayer,                     // baseArrayLayer
                range.layerCount                          // layerCount
        };
    }

    EResourceLayout VKToShiftResourceLayout(VkImageLayout layout) {
        return static_cast<EResourceLayout>(layout);
    }

    ETextureFormat VKToShiftTextureFormat(VkFormat format) {
        return static_cast<ETextureFormat>(format);
    }

    EAttachmentLoadOperation VKToShiftAttachmentLoadOperation(VkAttachmentLoadOp operation) {
        return static_cast<EAttachmentLoadOperation>(operation);
    }

    EAttachmentStoreOperation VKToShiftAttachmentStoreOperation(VkAttachmentStoreOp operation) {
        return static_cast<EAttachmentStoreOperation>(operation);
    }

    EStencilOp VKToShiftStencilOp(VkStencilOp op) {
        return static_cast<EStencilOp>(op);
    }

    EPolygonMode VKToShiftPolygonMode(VkPolygonMode mode) {
        return static_cast<EPolygonMode>(mode);
    }

    ECullMode VKToShiftCullMode(VkCullModeFlagBits mode) {
        return static_cast<ECullMode>(mode);
    }

    EWindingOrder VKToShiftWindingOrder(VkFrontFace order) {
        return static_cast<EWindingOrder>(order);
    }

    EColorWriteMask VKToShiftColorWriteMask(VkColorComponentFlags mask) {
        if (mask == (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT)) {
            return EColorWriteMask::RGB;
        } else if (mask == (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT)) {
            return EColorWriteMask::RGBA;
        }
        return static_cast<EColorWriteMask>(mask);
    }

    EBlendFactor VKToShiftBlendFactor(VkBlendFactor factor) {
        return static_cast<EBlendFactor>(factor);
    }

    EBlendOperation VKToShiftBlendOperation(VkBlendOp operation) {
        return static_cast<EBlendOperation>(operation);
    }

    EVertexInputRate VKToShiftVertexInputRate(VkVertexInputRate rate) {
        return static_cast<EVertexInputRate>(rate);
    }

    EPrimitiveTopology VKToShiftPrimitiveTopology(VkPrimitiveTopology topology) {
        return static_cast<EPrimitiveTopology>(topology);
    }

    EBindingType VKToShiftBindingType(VkDescriptorType bindingType) {
        return static_cast<EBindingType>(bindingType);
    }

    EBindingVisibility VKToShiftBindingVisibility(VkShaderStageFlagBits visibility) {
        return static_cast<EBindingVisibility>(visibility);
    }
} // Shift::VK::Util