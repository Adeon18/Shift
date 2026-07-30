//! Backend-specific tests for `Util::CreateImageMemoryBarrier2` - the one piece of a queue
//! ownership transfer pure enough to test without a device.
//!
//! Scope is deliberately narrow: only what the builder DERIVES is pinned here. Fields it copies
//! straight through (image, families, layouts, subresource range) are not asserted - a struct
//! copy cannot regress quietly, and checking it just restates the code in a second place.
//!
//! NOT covered: that the release and acquire halves agree in production. That pairing lives in
//! VK::CommandBuffer, needs a device, and any test written here would only be checking that the
//! test's own fixture passed matching arguments twice. The [app] suite exercises the real path.
//!
//! Suite tag: [vk] (filter with: shift_tests -ts=*[vk]*).
#ifdef SHIFT_VULKAN_BACKEND

#include <doctest/doctest.h>

#include "Utility/Vulkan/VKUtilInfo.hpp"

namespace VkUtil = Shift::VK::Util;

namespace {
    //! Never dereferenced, the builder only copies it into the barrier
    VkImage FakeImage() {
        return reinterpret_cast<VkImage>(static_cast<uintptr_t>(0xF00D));
    }

    VkImageSubresourceRange WholeColorImage() {
        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;
        return range;
    }

    constexpr uint32_t TRANSFER_FAMILY = 1;
    constexpr uint32_t GRAPHICS_FAMILY = 0;
    constexpr VkImageLayout UPLOAD_LAYOUT = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    constexpr VkImageLayout SAMPLED_LAYOUT = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

TEST_SUITE("QfotBarriers [vk]") {

//! The rule the whole handoff rests on, and the only non-obvious thing the builder does: a half
//! with no stage scope gets no access scope, INSTEAD of the mask its layout would imply.
//! Without it, the release of a SHADER_READ_ONLY image would name SHADER_READ on a transfer-only
//! family - a stage that family cannot represent - and contradict the spec's empty second scope
TEST_CASE("A NONE stage mask forces its access mask to NONE, whatever the layout says") {
    SUBCASE("release half: real source scope, empty destination scope") {
        const VkImageMemoryBarrier2 release = VkUtil::CreateImageMemoryBarrier2(
            FakeImage(), UPLOAD_LAYOUT, SAMPLED_LAYOUT,
            VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_PIPELINE_STAGE_2_NONE,
            TRANSFER_FAMILY, GRAPHICS_FAMILY, WholeColorImage());

        CHECK(release.srcAccessMask == VK_ACCESS_2_TRANSFER_WRITE_BIT);
        //! SAMPLED_LAYOUT would imply SHADER_READ here - the NONE stage must win
        CHECK(release.dstAccessMask == VK_ACCESS_2_NONE);
    }

    SUBCASE("acquire half: empty source scope, real destination scope") {
        const VkImageMemoryBarrier2 acquire = VkUtil::CreateImageMemoryBarrier2(
            FakeImage(), UPLOAD_LAYOUT, SAMPLED_LAYOUT,
            VK_PIPELINE_STAGE_2_NONE, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            TRANSFER_FAMILY, GRAPHICS_FAMILY, WholeColorImage());

        //! UPLOAD_LAYOUT would imply TRANSFER_WRITE here
        CHECK(acquire.srcAccessMask == VK_ACCESS_2_NONE);
        CHECK(acquire.dstAccessMask == VK_ACCESS_2_SHADER_READ_BIT);
    }
}

//! The contrast case: with real stages on both sides the access masks DO come from the layouts.
//! Pins the same-queue branch every ordinary transition takes
TEST_CASE("Real stages on both sides derive both access masks from the layouts") {
    const VkImageMemoryBarrier2 barrier = VkUtil::CreateImageMemoryBarrier2(
        FakeImage(), UPLOAD_LAYOUT, SAMPLED_LAYOUT,
        VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, WholeColorImage());

    CHECK(barrier.srcAccessMask == VK_ACCESS_2_TRANSFER_WRITE_BIT);
    CHECK(barrier.dstAccessMask == VK_ACCESS_2_SHADER_READ_BIT);
}

//! Depth is read-modify-write. Every other layout in the table maps to a single bit, so this is
//! the one entry where a plausible edit (dropping the READ) would still look right
TEST_CASE("Depth destination access keeps both the read and the write bit") {
    const VkAccessFlags2 depthAccess = VkUtil::LayoutToDstAccessMask2(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    CHECK(depthAccess == (VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT));
}

} // TEST_SUITE

#endif // SHIFT_VULKAN_BACKEND
