//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_COMMANDBUFFER_HPP
#define SHIFT_COMMANDBUFFER_HPP

#include <concepts>
#include <optional>
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

    struct DispatchConfig {
        uint32_t groupCountX = 1;
        uint32_t groupCountY = 1;
        uint32_t groupCountZ = 1;

        //! TODO: [FEATURE] this shoudl dissapear with reflection
        [[nodiscard]] static constexpr DispatchConfig CoveringThreads(
            uint32_t threadsX, uint32_t threadsY, uint32_t threadsZ,
            uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ)
        {
            const auto divideRoundUp = [](uint32_t value, uint32_t divisor) -> uint32_t {
                return divisor == 0u ? 0u : (value + divisor - 1u) / divisor;
            };
            return DispatchConfig{
                divideRoundUp(threadsX, groupSizeX),
                divideRoundUp(threadsY, groupSizeY),
                divideRoundUp(threadsZ, groupSizeZ)
            };
        }
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
    //! Both subresources name exactly ONE mip level: baseMipLevel is it, levelCount must be 1
    struct TextureBlitRegion {
        TextureSubresourceRange srcSubresource;
        Offset3D srcOffsets[2];
        TextureSubresourceRange destSubresource;
        Offset3D dstOffsets[2];
    };
    //! Texture Data that you pass in during the blitting process.
    //! The blit layouts are here as well as it turned out:D
    struct TextureBlitData {
        Texture* texture;
        EResourceLayout layout;
    };

    enum class EPoolQueueType {
        Graphics,
        Transfer,
        Compute,
    };

    //! A released texture waiting for its acquire half on dstQueue.
    //! The backend maps queues to whatever it needs ( I guess this is vk only but whatever lol)
    struct QueueOwnershipHandoffPayload {
        Texture* texture = nullptr;
        EPoolQueueType srcQueue = EPoolQueueType::Graphics;
        EPoolQueueType dstQueue = EPoolQueueType::Graphics;
        EResourceLayout oldLayout = EResourceLayout::Undefined;
        EResourceLayout newLayout = EResourceLayout::Undefined;
        EPipelineStageFlags dstStage = EPipelineStageFlags::NoneBit;
    };

    enum class ECommandBufferType {
        Default,
        Flight
    };

    struct SecondaryBufferBeginPayload {
        std::span<ETextureFormat> colorFormats;
        std::optional<ETextureFormat> depthFormat;
        std::optional<ETextureFormat> stencilFormat;
        //! TODO: [FEATURE]: Add MSAA sample counts here too, rn default is 1
    };

    template<typename CommandBuffer>
    concept ICommandBuffer =
        std::constructible_from<CommandBuffer, const Device* /*device*/, const Instance* /*ins*/, const CommandPool& /*commandPool*/, bool /*isSecondary*/> &&
        std::destructible<CommandBuffer> &&
    requires(
            CommandBuffer InputBuffer,
            const RenderPass& InputPass,
            const BufferOpDescriptor& InputBufferOpDesc,
            const TextureCopyDescriptor& InputTextureCopyDesc,
            const Pipeline& InputPipeline,
            const ResourceSet& InputResourceSet,
            const DrawConfig& InputDrawConfig,
            const DrawIndexedConfig& InputDrawIndexedConfig,
            const DispatchConfig& InputDispatchConfig,
            const MemoryDependency& InputMemoryDependency,
            const Viewport& InputViewport,
            const Rect2D& InputScissor,
            const TextureBlitData& InputTextureBlitData,
            const TextureBlitRegion& InputBlitRegion,
            Texture& InputTexture,
            const TextureSubresourceRange& InputSubresourceRange,
            const QueueOwnershipHandoffPayload& InputHandoff,
            EPoolQueueType InputQueueType,
            EResourceLayout InputLayout,
            EPipelineStageFlags InputStage,
            std::span<BinarySemaphore*> InputBinSemaphoreSpan,
            std::span<TimelineSemaphore*> InputTimeSemaphoreSpan,
            std::span<uint64_t> InputTimeSemaphoreCounter,
            std::span<BufferOpDescriptor> InputBufferOpDescs,
            std::span<ResourceSet> InputResourceSets,
            std::span<CommandBuffer*> InputCommandBuffers,
            const SecondaryBufferBeginPayload& secondaryBufferBeginPayload,
            uint32_t firstBindPosition,
            uint32_t size,
            uint32_t fillValue,
            EFilterMode filter,
            EIndexSize indexSize
    ) {
        //! Basic
        { InputBuffer.Begin() } -> std::same_as<bool>;
        { InputBuffer.BeginSecondary(secondaryBufferBeginPayload) } -> std::same_as<bool>;
        { InputBuffer.End() } -> std::same_as<bool>;
        { InputBuffer.Reset() } -> std::same_as<void>;
        //! These are per-backend specific
        //!{ InputBuffer.BeginRenderPass(InputPass) } -> std::same_as<void>;
        //!{ InputBuffer.EndRenderPass() } -> std::same_as<void>;
        { InputBuffer.ExecuteSecondaryBuffers(InputCommandBuffers) } -> std::same_as<void>;
        { InputBuffer.Submit(InputTimeSemaphoreSpan, InputTimeSemaphoreCounter, InputTimeSemaphoreSpan, InputTimeSemaphoreCounter) } -> std::same_as<bool>;
        { InputBuffer.Submit(InputTimeSemaphoreSpan, InputTimeSemaphoreCounter, InputTimeSemaphoreSpan, InputTimeSemaphoreCounter, InputBinSemaphoreSpan, InputBinSemaphoreSpan) } -> std::same_as<bool>;
        { InputBuffer.CopyBufferToBuffer(InputBufferOpDesc, InputBufferOpDesc, size) } -> std::same_as<void>;
        { InputBuffer.CopyBufferToTexture(InputBufferOpDesc, InputTextureCopyDesc) } -> std::same_as<void>;
        { InputBuffer.CopyTextureToBuffer(InputTextureCopyDesc, InputBufferOpDesc) } -> std::same_as<void>;
        { InputBuffer.BufferBarrier(InputBufferOpDesc, size, InputMemoryDependency) } -> std::same_as<void>;
        { InputBuffer.FillBuffer(InputBufferOpDesc, size, fillValue) } -> std::same_as<void>;
        //{ InputBuffer.CopyTextureToTexture(InputTextureCopyDesc, InputTextureCopyDesc) } -> std::same_as<void>; // TODO: [FEATURE] Check
        //! Rendering
        { InputBuffer.BindGraphicsPipeline(InputPipeline) } -> std::same_as<void>;
        { InputBuffer.BindVertexBuffer(InputBufferOpDesc, firstBindPosition) } -> std::same_as<void>;
        { InputBuffer.BindVertexBuffers(InputBufferOpDescs, firstBindPosition) } -> std::same_as<void>;
        { InputBuffer.BindIndexBuffer(InputBufferOpDesc, indexSize) } -> std::same_as<void>;
        { InputBuffer.BindResourceSet(InputPipeline, firstBindPosition, InputResourceSet) } -> std::same_as<void>;
        //!{ InputBuffer.BindResourceSets(InputPipeline, firstBindPosition, InputResourceSets) } -> std::same_as<void>; // Dynamic offsets will be pulled out of my fucking ass
        { InputBuffer.Draw(InputDrawConfig) } -> std::same_as<void>;
        { InputBuffer.DrawIndexed(InputDrawIndexedConfig) } -> std::same_as<void>;
        //! Compute
        { InputBuffer.BindComputePipeline(InputPipeline) } -> std::same_as<void>;
        { InputBuffer.Dispatch(InputDispatchConfig) } -> std::same_as<void>;
        //! Misc
        { InputBuffer.SetViewport(InputViewport) } -> std::same_as<void>;
        { InputBuffer.SetScissor(InputScissor) } -> std::same_as<void>;
        { InputBuffer.BlitTexture(InputTextureBlitData, InputTextureBlitData, InputBlitRegion, filter) } -> std::same_as<void>;
        { InputBuffer.GenerateMips(InputTexture, InputLayout, InputStage, filter) } -> std::same_as<void>;
        //! Barriers
        { InputBuffer.TextureBarrier(InputTexture, InputMemoryDependency, InputLayout, InputLayout, InputSubresourceRange) } -> std::same_as<void>;
        { CONCEPT_CONST_VAR(CommandBuffer, InputBuffer).NeedsOwnershipTransfer(InputQueueType) } -> std::same_as<bool>;
        { InputBuffer.ReleaseQueueOwnership(InputTexture, InputLayout, InputStage, InputQueueType, InputLayout, InputStage) } -> std::same_as<void>;
        { InputBuffer.AcquireQueueOwnership(InputHandoff) } -> std::same_as<void>;
    };

    template<typename CommandPool>
    concept ICommandPool =
        std::constructible_from<CommandPool, const Device*, EPoolQueueType> &&
        std::destructible<CommandPool> &&
    requires(
            CommandPool InputPool
    )
    {
        { InputPool.GetType() } -> std::same_as<EPoolQueueType>;
        { InputPool.Reset() } -> std::same_as<void>;
    };
} // Shift

#endif //SHIFT_COMMANDBUFFER_HPP
