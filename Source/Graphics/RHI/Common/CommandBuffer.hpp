//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_COMMANDBUFFER_HPP
#define SHIFT_COMMANDBUFFER_HPP

#include <concepts>
#include <type_traits>
#include <span>

#include "Base.hpp"
#include "Types.hpp"
#include "Texture.hpp"
#include "Sampler.hpp"

namespace Shift {
    //! With instanceCount = 1, it is regular draw, else it is instanced draw
    struct DrawConfig{
        uint32_t vertexCount = 0;
        uint32_t instanceCount = 1;
        uint32_t firstVertex = 0;
        uint32_t firstInstance = 0;
    };

    //! With instanceCount = 1, it is regular draw, else it is instanced draw
    struct DrawIndexedConfig{
        uint32_t indexCount = 0;
        uint32_t instanceCount = 1;
        uint32_t firstIndex = 0;
        int32_t vertexOffset = 0;
        uint32_t firstInstance = 0;
    };

    //! Buffer data for a copy operation (based on Vulkan)
    struct BufferOpDescriptor {
        Buffer* buffer;
        uint32_t offset;
    };

    //! Almost 1:1 with Vulkan
    enum class EIndexSize {
        UInt16 = 0,
        UInt32 = 1,
        UInt8 = 2,
    };

    //! Texture data for a copy operation (based on Vulkan)
    struct TextureCopyDescriptor {
        Texture* texture;
        Extent3D size;
        Offset3D offset;
        TextureSubresourceRange subresourceRange;
    };

    //! The region struct for blitting a texture (useful for mipmapping)
    struct TextureBlitRegion {
        TextureSubresourceRange srcSubresource;
        Offset3D srcOffsets[2];
        TextureSubresourceRange destSubresource;
        Offset3D dstOffsets[2];
    };

    //! Texture Data that you pass in during the blitting process
    struct TextureBlitData {
        Texture* texture;
        EResourceLayout layout;
    };

    enum class EPoolQueueType {
        Graphics,
        Transfer,
        Compute,
    };

    enum class ECommandBufferType {
        Default,
        Flight
    };

    template<typename CommandBuffer>
    concept ICommandBuffer =
        std::is_default_constructible_v<CommandBuffer> &&
        std::is_trivially_destructible_v<CommandBuffer> &&
    requires(
            CommandBuffer InputBuffer,
            const RenderPass& InputPass,
            const BufferOpDescriptor& InputBufferOpDesc,
            const TextureCopyDescriptor& InputTextureCopyDesc,
            const Pipeline& InputPipeline,
            const ResourceSet& InputResourceSet,
            const DrawConfig& InputDrawConfig,
            const DrawIndexedConfig& InputDrawIndexedConfig,
            const Viewport& InputViewport,
            const Rect2D& InputScissor,
            const TextureBlitData& InputTextureBlitData,
            const TextureBlitRegion& InputBlitRegion,
            std::span<BinarySemaphore*> InputBinSemaphoreSpan,
            std::span<TimelineSemaphore*> InputTimeSemaphoreSpan,
            std::span<uint64_t> InputTimeSemaphoreCounter,
            std::span<BufferOpDescriptor> InputBufferOpDescs,
            std::span<ResourceSet> InputResourceSets,
            uint32_t firstBindPosition,
            uint32_t size,
            EFilterMode filter,
            EIndexSize indexSize
    ) {
        //! Basic
        { InputBuffer.Begin() } -> std::same_as<bool>;
        { InputBuffer.End() } -> std::same_as<bool>;
        { InputBuffer.Reset() } -> std::same_as<void>;
        //! These are per-backend specific
        //!{ InputBuffer.BeginRenderPass(InputPass) } -> std::same_as<void>;
        //!{ InputBuffer.EndRenderPass() } -> std::same_as<void>;
        { InputBuffer.Submit(InputTimeSemaphoreSpan, InputTimeSemaphoreCounter, InputTimeSemaphoreSpan, InputTimeSemaphoreCounter) } -> std::same_as<bool>;
        { InputBuffer.Submit(InputTimeSemaphoreSpan, InputTimeSemaphoreCounter, InputTimeSemaphoreSpan, InputTimeSemaphoreCounter, InputBinSemaphoreSpan, InputBinSemaphoreSpan) } -> std::same_as<bool>;
        { InputBuffer.CopyBufferToBuffer(InputBufferOpDesc, InputBufferOpDesc, size) } -> std::same_as<void>;
        { InputBuffer.CopyBufferToTexture(InputBufferOpDesc, InputTextureCopyDesc) } -> std::same_as<void>;
        //{ InputBuffer.CopyTextureToBuffer(InputTextureCopyDesc, InputBufferOpDesc, size) } -> std::same_as<void>; // TODO: [FEATURE] Check
        //{ InputBuffer.CopyTextureToTexture(InputTextureCopyDesc, InputTextureCopyDesc) } -> std::same_as<void>; // TODO: [FEATURE] Check
        //! Rendering
        { InputBuffer.BindGraphicsPipeline(InputPipeline) } -> std::same_as<void>;
        { InputBuffer.BindVertexBuffer(InputBufferOpDesc, firstBindPosition) } -> std::same_as<void>;
        { InputBuffer.BindVertexBuffers(InputBufferOpDescs, firstBindPosition) } -> std::same_as<void>;
        { InputBuffer.BindIndexBuffer(InputBufferOpDesc, indexSize) } -> std::same_as<void>;
        // These will probably be per-backend specific too
        //!{ InputBuffer.BindResourceSet(InputResourceSet, firstBindPosition) } -> std::same_as<void>;     // Dynamic offsets will be pulled out of my fucking ass
        //!{ InputBuffer.BindResourceSets(InputResourceSets, firstBindPosition) } -> std::same_as<void>;
        { InputBuffer.Draw(InputDrawConfig) } -> std::same_as<void>;
        { InputBuffer.DrawIndexed(InputDrawIndexedConfig) } -> std::same_as<void>;
        //! Misc
        { InputBuffer.SetViewport(InputViewport) } -> std::same_as<void>;
        { InputBuffer.SetScissor(InputScissor) } -> std::same_as<void>;
        { InputBuffer.BlitTexture(InputTextureBlitData, InputTextureBlitData, InputBlitRegion, filter) } -> std::same_as<void>;
    };
} // Shift

#endif //SHIFT_COMMANDBUFFER_HPP
