#include "Sync.hpp"

#include "Utility/Logging/LogMacros.hpp"

namespace Shift {
    EAccessFlags LayoutToSrcAccess(EResourceLayout layout) {
        switch (layout) {
            case EResourceLayout::Undefined:
                return EAccessFlags::NoneBit;
            case EResourceLayout::ColorAttachmentOptimal:
                return EAccessFlags::ColorAttachmentWriteBit;
            case EResourceLayout::DepthStencilAttachmentOptimal:
                return EAccessFlags::DepthStencilAttachmentWriteBit;
            case EResourceLayout::TransferSrcOptimal:
                return EAccessFlags::TransferReadBit;
            case EResourceLayout::TransferDstOptimal:
                return EAccessFlags::TransferWriteBit;
            case EResourceLayout::ShaderReadOnlyOptimal:
                return EAccessFlags::ShaderReadBit;
            case EResourceLayout::Present:
                //! Nothing to wait for, the presentation engine's read is ordered by the semaphore
                return EAccessFlags::NoneBit;
            default:
                Log(Error, "Unsupported source layout!");
                return EAccessFlags::NoneBit;
        }
    }

    EAccessFlags LayoutToDstAccess(EResourceLayout layout) {
        switch (layout) {
            case EResourceLayout::TransferDstOptimal:
                return EAccessFlags::TransferWriteBit;
            case EResourceLayout::TransferSrcOptimal:
                return EAccessFlags::TransferReadBit;
            case EResourceLayout::ColorAttachmentOptimal:
                //! loadOp = Load and blending read the attachment
                return EAccessFlags::ColorAttachmentReadBit | EAccessFlags::ColorAttachmentWriteBit;
            case EResourceLayout::DepthStencilAttachmentOptimal:
                return EAccessFlags::DepthStencilAttachmentReadBit | EAccessFlags::DepthStencilAttachmentWriteBit;
            case EResourceLayout::ShaderReadOnlyOptimal:
                return EAccessFlags::ShaderReadBit;
            case EResourceLayout::Present:
                return EAccessFlags::NoneBit;
            default:
                Log(Error, "Unsupported destination layout!");
                return EAccessFlags::NoneBit;
        }
    }

    MemoryDependency MakeTextureDependency(EResourceLayout oldLayout, EPipelineStageFlags srcStage, EResourceLayout newLayout, EPipelineStageFlags dstStage) {
        return MemoryDependency{
            .srcStage = srcStage,
            .srcAccess = (srcStage == EPipelineStageFlags::NoneBit) ? EAccessFlags::NoneBit : LayoutToSrcAccess(oldLayout),
            .dstStage = dstStage,
            .dstAccess = (dstStage == EPipelineStageFlags::NoneBit) ? EAccessFlags::NoneBit : LayoutToDstAccess(newLayout),
        };
    }
} // Shift
