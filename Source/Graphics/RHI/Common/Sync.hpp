#ifndef SHIFT_SYNC_HPP
#define SHIFT_SYNC_HPP

#include "Base.hpp"

namespace Shift {
    //! 1:1 with Vulkan (not everything is included)
    enum class EResourceLayout {
        Undefined = 0,
        General = 1,
        ColorAttachmentOptimal = 2,
        DepthStencilAttachmentOptimal = 3,
        DepthStencilReadOnlyOptimal = 4,
        ShaderReadOnlyOptimal = 5,
        TransferSrcOptimal = 6,
        TransferDstOptimal = 7,
        Preinitialized = 8,
        Present = 1000001002,
    };


    //! 1:1 with VkPipelineStageFlagBits2 (sync2, 64-bit)
    enum class EPipelineStageFlags : uint64_t
    {
        NoneBit                        = 0,
        TopOfPipeBit                   = 1ull << 0,
        DrawIndirectBit                = 1ull << 1,
        VertexInputBit                 = 1ull << 2,
        VertexShaderBit                = 1ull << 3,
        TessellationControlShaderBit   = 1ull << 4,
        TessellationEvaluationShaderBit= 1ull << 5,
        GeometryShaderBit              = 1ull << 6,
        FragmentShaderBit              = 1ull << 7,
        EarlyFragmentTestsBit          = 1ull << 8,
        LateFragmentTestsBit           = 1ull << 9,
        ColorAttachmentOutputBit       = 1ull << 10,
        ComputeShaderBit               = 1ull << 11,
        AllTransferBit                 = 1ull << 12,
        BottomOfPipeBit                = 1ull << 13,
        HostBit                        = 1ull << 14,
        AllGraphicsBit                 = 1ull << 15,
        AllCommandsBit                 = 1ull << 16,

        // New in sync2
        CopyBit                        = 1ull << 32,
        ResolveBit                     = 1ull << 33,
        BlitBit                        = 1ull << 34,
        ClearBit                       = 1ull << 35,
        IndexInputBit                  = 1ull << 36,
        VertexAttributeInputBit        = 1ull << 37,
        PreRasterizationShadersBit     = 1ull << 38
    };
    DEFINE_ENUM_CLASS_BITWISE_OPERATORS(EPipelineStageFlags);

    //! 1:1 with VkAccessFlagBits2, and present in dx12 as well
    enum class EAccessFlags : uint64_t
    {
        NoneBit                          = 0,
        IndirectCommandReadBit           = 1ull << 0,
        IndexReadBit                     = 1ull << 1,
        VertexAttributeReadBit           = 1ull << 2,
        UniformReadBit                   = 1ull << 3,
        InputAttachmentReadBit           = 1ull << 4,
        ShaderReadBit                    = 1ull << 5,
        ShaderWriteBit                   = 1ull << 6,
        ColorAttachmentReadBit           = 1ull << 7,
        ColorAttachmentWriteBit          = 1ull << 8,
        DepthStencilAttachmentReadBit    = 1ull << 9,
        DepthStencilAttachmentWriteBit   = 1ull << 10,
        TransferReadBit                  = 1ull << 11,
        TransferWriteBit                 = 1ull << 12,
        HostReadBit                      = 1ull << 13,
        HostWriteBit                     = 1ull << 14,
        MemoryReadBit                    = 1ull << 15,
        MemoryWriteBit                   = 1ull << 16,

        //! sync-2
        ShaderSampledReadBit             = 1ull << 32,
        ShaderStorageReadBit             = 1ull << 33,
        ShaderStorageWriteBit            = 1ull << 34
    };
    DEFINE_ENUM_CLASS_BITWISE_OPERATORS(EAccessFlags);

    //! Every access that writes memory
    inline constexpr EAccessFlags ACCESS_ALL_WRITE_MASK =
        EAccessFlags::ShaderWriteBit |
        EAccessFlags::ColorAttachmentWriteBit |
        EAccessFlags::DepthStencilAttachmentWriteBit |
        EAccessFlags::TransferWriteBit |
        EAccessFlags::HostWriteBit |
        EAccessFlags::MemoryWriteBit |
        EAccessFlags::ShaderStorageWriteBit;

    //! The elements that every barrier form uses, buffer, texture (+layout) and global
    struct MemoryDependency {
        EPipelineStageFlags srcStage = EPipelineStageFlags::NoneBit;
        EAccessFlags        srcAccess = EAccessFlags::NoneBit;
        EPipelineStageFlags dstStage = EPipelineStageFlags::NoneBit;
        EAccessFlags        dstAccess = EAccessFlags::NoneBit;
    };

    //! Texture layout to access flags
    EAccessFlags LayoutToSrcAccess(EResourceLayout layout);

    //! TODO [CLEANUP] this had code duplication but fuck it
    EAccessFlags LayoutToDstAccess(EResourceLayout layout);

    //! Agnostic dep creator, DO NOT CREATE THEM URSELF UNLESS YPOU WANT TO HANDLE HALF NON DEST ACCESSES FOR QUEUE TRANSITIONS (2 TERNARIES)
    MemoryDependency MakeTextureDependency(EResourceLayout oldLayout, EPipelineStageFlags srcStage, EResourceLayout newLayout, EPipelineStageFlags dstStage);

    template<typename API> class ResourceStateTracker;

    //! A texture's state as of its last successful submit. Every backend texture must store one, only the tracker reads and writes it
    //! The alternativew are getters/setters in texture per API and I did not like that
    class TextureSyncState {
        template<typename API> friend class ResourceStateTracker;

        EResourceLayout layout = EResourceLayout::Undefined;
        EPipelineStageFlags stage = EPipelineStageFlags::TopOfPipeBit;
    };

    //! TODO: [CONSPECT] A buffer's state as of its last successful submit, whole buffer. Same rules as TextureSyncState
    //! Since buffers have no layouts, we need to store both read and write states, as:
    //! 1. For a read, we need to wait for a write. 2. For a write, we need mem dep for a write before to order and if write after read - need exec dependency so that data is know overriden
    //! For textures, the layout handles this, as a layout change always means 1 or 2 behaviour from above - hence a barrier is needed and the read after read is ofc a no-barrier
    //! and this is too the reason we can infer stage from layout!
    class BufferSyncState {
        template<typename API> friend class ResourceStateTracker;
        //! NoneBit while the GPU never wrote the buffer
        EPipelineStageFlags writeStage = EPipelineStageFlags::NoneBit;
        EAccessFlags writeAccess = EAccessFlags::NoneBit;
        //! Every read since that write, it is OR-ED into the read stages as the next write has to wait for all of them
        EPipelineStageFlags readStages = EPipelineStageFlags::NoneBit;
        EAccessFlags readAccess = EAccessFlags::NoneBit;
    };
} // Shift

#endif //SHIFT_SYNC_HPP
