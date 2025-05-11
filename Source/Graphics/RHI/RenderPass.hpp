//
// Created by otrush on 10/23/2024.
//

#ifndef SHIFT_RENDERPASS_HPP
#define SHIFT_RENDERPASS_HPP

#include <concepts>
#include <type_traits>
#include <vector>
#include <optional>

#include "Base.hpp"

#include "TextureFormat.hpp"
#include "Pipeline.hpp"

namespace Shift {
    //! 1:1 with Vulkan
    enum class EAttachmentLoadOperation {
        Load,
        Clear,
        DontCare
    };

    //! 1:1 with Vulkan
    enum class EAttachmentStoreOperation {
        Store,
        DontCare
    };

    //! 1:1 with Vulkan
    union UClearColor {
        float       float32[4];
        int32_t     int32[4];
        uint32_t    uint32[4];
    };

    //! 1:1 with Vulkan
    struct ClearDepthStencil {
        float depth;
        uint32_t stencil;
    };

    //! 1:1 with Vulkan
    struct AttachmentClearValue {
        UClearColor color;
        ClearDepthStencil depthStencil;
    };

    //! The descriptor struct for a render pass. The render target is specified by name as it is an offline struct
    //! and the RT should be pulled by the graphics context by name at the render pass creation.
    //! Note: this does not follow the laws of VkRenderPass but rather the VK_KHR_Dynamic_Rendering extension
    //! You can have multiple pipelines/draw calls that fit the same render pass
    struct RenderPassDescriptor {
        struct RenderPassAttachmentInfo {
            const char* renderTargetName = "EMPTY";
            EResourceLayout renderTargetLayout = EResourceLayout::ColorAttachmentOptimal;
            EAttachmentLoadOperation loadOperation = EAttachmentLoadOperation::Clear;
            EAttachmentStoreOperation storeOperation = EAttachmentStoreOperation::Store;
            AttachmentClearValue clearValue = {
                    .color={0.0f, 0.0f, 0.0f, 1.0f},
                    .depthStencil={1.0f, 0u},
            };
        };

        std::vector<RenderPassAttachmentInfo> colorAttachments;
        std::optional<RenderPassAttachmentInfo> depthAttachment;
    };

    //! RenderPass interface is largely free and up for implementation as different APIs have different implementation
    //! caveats.
    //! NE NOTE - This is probably not needed as I can just pass the renderpass desc to the command buffer - it has everything
//    template<typename RenderPass>
//    concept IRenderPass =
//        std::is_default_constructible_v<RenderPass> &&
//        std::is_trivially_destructible_v<RenderPass> &&
//    requires(RenderPass InputRenderPass, const Device* device, const RenderPassDescriptor& desc) {
//        { InputRenderPass.Init(device, desc) } -> std::same_as<bool>;
//    };
} // Shift

#endif //SHIFT_RENDERPASS_HPP
