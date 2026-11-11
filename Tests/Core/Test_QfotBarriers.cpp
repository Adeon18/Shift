//! Tests for `MakeTextureDependency` - the agnostic layout -> access derivation every texture
//! barrier goes through, including both halves of a queue ownership transfer.
//!
//! Scope is deliberately narrow: only what the function DERIVES is pinned here. Stages are copied
//! straight through and are not asserted - a struct copy cannot regress quietly, and checking it
//! just restates the code in a second place.
//!
//! NOT covered: that the release and acquire halves agree in production. That pairing lives in
//! the tracker + VK::CommandBuffer, needs a device, and any test written here would only be
//! checking that the test's own fixture passed matching arguments twice. The [app] suite exercises
//! the real path.
#include <doctest/doctest.h>

#include "Graphics/RHI/Common/Sync.hpp"

using namespace Shift;

namespace {
    constexpr EResourceLayout UPLOAD_LAYOUT = EResourceLayout::TransferDstOptimal;
    constexpr EResourceLayout SAMPLED_LAYOUT = EResourceLayout::ShaderReadOnlyOptimal;
}

TEST_SUITE("QfotBarriers") {

//! The rule the whole handoff rests on, and the only non-obvious thing the derivation does: a half
//! with no stage scope gets no access scope, INSTEAD of the mask its layout would imply.
//! Without it, the release of a SHADER_READ_ONLY image would name SHADER_READ on a transfer-only
//! family - a stage that family cannot represent - and contradict the spec's empty second scope
TEST_CASE("A None stage forces its access to None, whatever the layout says") {
    SUBCASE("release half: real source scope, empty destination scope") {
        const MemoryDependency release = MakeTextureDependency(
            UPLOAD_LAYOUT, EPipelineStageFlags::AllTransferBit,
            SAMPLED_LAYOUT, EPipelineStageFlags::NoneBit);

        CHECK(release.srcAccess == EAccessFlags::TransferWriteBit);
        //! SAMPLED_LAYOUT would imply ShaderRead here - the None stage must win
        CHECK(release.dstAccess == EAccessFlags::NoneBit);
    }

    SUBCASE("acquire half: empty source scope, real destination scope") {
        const MemoryDependency acquire = MakeTextureDependency(
            UPLOAD_LAYOUT, EPipelineStageFlags::NoneBit,
            SAMPLED_LAYOUT, EPipelineStageFlags::FragmentShaderBit);

        //! UPLOAD_LAYOUT would imply TransferWrite here
        CHECK(acquire.srcAccess == EAccessFlags::NoneBit);
        CHECK(acquire.dstAccess == EAccessFlags::ShaderReadBit);
    }
}

//! The contrast case: with real stages on both sides the access masks DO come from the layouts.
//! Pins the same-queue branch every ordinary transition takes
TEST_CASE("Real stages on both sides derive both access masks from the layouts") {
    const MemoryDependency dependency = MakeTextureDependency(
        UPLOAD_LAYOUT, EPipelineStageFlags::AllTransferBit,
        SAMPLED_LAYOUT, EPipelineStageFlags::FragmentShaderBit);

    CHECK(dependency.srcAccess == EAccessFlags::TransferWriteBit);
    CHECK(dependency.dstAccess == EAccessFlags::ShaderReadBit);
}

//! Depth is read-modify-write. Every other layout in the table maps to a single bit, so this is
//! the one entry where a plausible edit (dropping the READ) would still look right
TEST_CASE("Depth destination access keeps both the read and the write bit") {
    const EAccessFlags depthAccess = LayoutToDstAccess(EResourceLayout::DepthStencilAttachmentOptimal);
    CHECK(depthAccess == (EAccessFlags::DepthStencilAttachmentReadBit | EAccessFlags::DepthStencilAttachmentWriteBit));
}

} // TEST_SUITE
