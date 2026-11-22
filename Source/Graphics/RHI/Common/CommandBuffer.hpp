//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_COMMANDBUFFER_HPP
#define SHIFT_COMMANDBUFFER_HPP

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>
#include <span>
#include <vector>

#include "Base.hpp"
#include "Capabilities.hpp"
#include "GPUProfiling.hpp"
#include "RenderPass.hpp"
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
        //! Bytes between rows on the buffer side, 0 = tightly packed. RHI::GetTextureCopyFootprint gives the one the device needs
        uint32_t bufferRowPitch = 0;
    };

    //! The buffer layout a texture copy needs on this device, rows are rowPitch bytes apart
    struct TextureCopyFootprint {
        uint32_t rowSize = 0;
        uint32_t rowPitch = 0;
        uint32_t rows = 0;
        uint64_t size = 0;
    };

    namespace RHIDetail {
        inline void CopyRows(uint8_t* dst, size_t dstPitch, const uint8_t* src, size_t srcPitch, size_t rowSize, size_t rows) {
            for (size_t row = 0; row < rows; ++row) {
                std::memcpy(dst + row * dstPitch, src + row * srcPitch, rowSize);
            }
        }
    } // RHIDetail

    //! Tight rows into a buffer laid out by the footprint, as many whole rows as src holds
    inline void CopyToFootprint(void* dst, const TextureCopyFootprint& footprint, std::span<const uint8_t> src) {
        const size_t rows = std::min<size_t>(footprint.rows, src.size() / footprint.rowSize);
        RHIDetail::CopyRows(static_cast<uint8_t*>(dst), footprint.rowPitch, src.data(), footprint.rowSize, footprint.rowSize, rows);
    }

    //! A buffer laid out by the footprint back into tight rows
    inline std::vector<uint8_t> CopyFromFootprint(const void* src, const TextureCopyFootprint& footprint) {
        std::vector<uint8_t> tight(size_t(footprint.rowSize) * footprint.rows);
        RHIDetail::CopyRows(tight.data(), footprint.rowSize, static_cast<const uint8_t*>(src), footprint.rowPitch, footprint.rowSize, footprint.rows);
        return tight;
    }

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
        std::destructible<CommandBuffer> &&
    requires(
            CommandBuffer InputBuffer,
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
            Texture& InputTexture,
            const TextureSubresourceRange& InputSubresourceRange,
            EResourceLayout InputLayout,
            EPipelineStageFlags InputStage,
            std::span<TimelineSemaphore*> InputTimeSemaphoreSpan,
            std::span<uint64_t> InputTimeSemaphoreCounter,
            std::span<BufferOpDescriptor> InputBufferOpDescs,
            std::span<ResourceSet> InputResourceSets,
            std::span<CommandBuffer*> InputCommandBuffers,
            const SecondaryBufferBeginPayload& secondaryBufferBeginPayload,
            const RenderPassDescriptor& InputPassDesc,
            std::span<Texture*> InputColorTextures,
            std::span<const EResourceLayout> InputColorLayouts,
            std::optional<Texture*> InputDepthTexture,
            const char* InputName,
            const DebugLabelColor& InputColor,
            const void* InputData,
            bool timed,
            uint32_t firstBindPosition,
            uint32_t size,
            uint32_t fillValue,
            EFilterMode filter,
            EIndexSize indexSize
    ) {
        //! Basic
        { CONCEPT_CONST_VAR(CommandBuffer, InputBuffer).IsValid() } -> std::same_as<bool>;
        { CONCEPT_CONST_VAR(CommandBuffer, InputBuffer).SetDebugName(InputName) } -> std::same_as<void>;
        { InputBuffer.Begin() } -> std::same_as<bool>;
        { InputBuffer.BeginSecondary(secondaryBufferBeginPayload) } -> std::same_as<bool>;
        { InputBuffer.End() } -> std::same_as<bool>;
        { InputBuffer.Reset() } -> std::same_as<void>;
        { InputBuffer.ExecuteSecondaryBuffers(InputCommandBuffers) } -> std::same_as<void>;
        { CONCEPT_CONST_VAR(CommandBuffer, InputBuffer).GetPoolType() } -> std::same_as<EPoolQueueType>;
        { InputBuffer.Submit(InputTimeSemaphoreSpan, InputTimeSemaphoreCounter, InputTimeSemaphoreSpan, InputTimeSemaphoreCounter) } -> std::same_as<bool>;
        { InputBuffer.CopyBufferToBuffer(InputBufferOpDesc, InputBufferOpDesc, size) } -> std::same_as<void>;
        { InputBuffer.CopyBufferToTexture(InputBufferOpDesc, InputTextureCopyDesc) } -> std::same_as<void>;
        { InputBuffer.CopyTextureToBuffer(InputTextureCopyDesc, InputBufferOpDesc) } -> std::same_as<void>;
        { InputBuffer.BufferBarrier(InputBufferOpDesc, size, InputMemoryDependency) } -> std::same_as<void>;
        { InputBuffer.FillBuffer(InputBufferOpDesc, size, fillValue) } -> std::same_as<void>;
        //{ InputBuffer.CopyTextureToTexture(InputTextureCopyDesc, InputTextureCopyDesc) } -> std::same_as<void>; // TODO: [FEATURE] Check
        //! Rendering
        { InputBuffer.BeginRenderPass(InputPassDesc, InputColorTextures, InputColorLayouts, InputDepthTexture, InputLayout) } -> std::same_as<void>;
        { InputBuffer.EndRenderPass() } -> std::same_as<void>;
        { InputBuffer.BindGraphicsPipeline(InputPipeline) } -> std::same_as<void>;
        { InputBuffer.SetPushConstants(InputPipeline, InputData, size, size) } -> std::same_as<void>;
        { InputBuffer.BindVertexBuffer(InputPipeline, InputBufferOpDesc, firstBindPosition) } -> std::same_as<void>;
        { InputBuffer.BindVertexBuffers(InputPipeline, InputBufferOpDescs, firstBindPosition) } -> std::same_as<void>;
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
        { InputBuffer.GenerateMips(InputTexture, InputLayout, InputStage, filter) } -> std::same_as<void>;
        //! Debug + timing
        { InputBuffer.PushDebugGroup(InputName, InputColor, timed) } -> std::same_as<void>;
        { InputBuffer.PopDebugGroup() } -> std::same_as<void>;
        { InputBuffer.InsertDebugLabel(InputName, InputColor) } -> std::same_as<void>;
        { InputBuffer.PushTimeRange(InputName) } -> std::same_as<void>;
        { InputBuffer.PopTimeRange() } -> std::same_as<void>;
        { InputBuffer.CollectTimeRanges() } -> std::same_as<std::vector<GPUTimeRange>>;
        //! Barriers
        { InputBuffer.TextureBarrier(InputTexture, InputMemoryDependency, InputLayout, InputLayout, InputSubresourceRange) } -> std::same_as<void>;
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
