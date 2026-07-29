//! Backend-specific tests: queue family policy, pinned against synthetic driver
//! tables. SelectQueueFamilies is pure, so every vendor layout that matters is two lines here
//! instead of a GPU we do not own. Suite tag: [vk] (filter with: shift_tests -ts=*[vk]*).
#ifdef SHIFT_VULKAN_BACKEND

#include <doctest/doctest.h>

#include <vector>

#include "Utility/Vulkan/VKUtilCore.hpp"

namespace VkUtil = Shift::VK::Util;

namespace {
    //! queueCount is only ever checked for "is this family usable at all", so one queue is enough
    VkQueueFamilyProperties Family(VkQueueFlags flags, uint32_t queueCount = 1) {
        VkQueueFamilyProperties props{};
        props.queueFlags = flags;
        props.queueCount = queueCount;
        props.timestampValidBits = 64;
        props.minImageTransferGranularity = VkExtent3D{1, 1, 1};
        return props;
    }

    constexpr VkQueueFlags GRAPHICS = VK_QUEUE_GRAPHICS_BIT;
    constexpr VkQueueFlags COMPUTE  = VK_QUEUE_COMPUTE_BIT;
    constexpr VkQueueFlags TRANSFER = VK_QUEUE_TRANSFER_BIT;
    constexpr VkQueueFlags SPARSE   = VK_QUEUE_SPARSE_BINDING_BIT;
    constexpr VkQueueFlags VIDEO_DECODE = VK_QUEUE_VIDEO_DECODE_BIT_KHR;
    constexpr VkQueueFlags VIDEO_ENCODE = VK_QUEUE_VIDEO_ENCODE_BIT_KHR;
    constexpr VkQueueFlags OPTICAL_FLOW = VK_QUEUE_OPTICAL_FLOW_BIT_NV;

    //! The table this machine actually reports (RTX 5070 Laptop, 6 families). Families 0/1/2
    //! present; the video and optical-flow engines do not
    std::vector<VkQueueFamilyProperties> Rtx5070Families() {
        return {
            Family(GRAPHICS | COMPUTE | TRANSFER | SPARSE, 16),
            Family(TRANSFER | SPARSE, 2),
            Family(COMPUTE | TRANSFER | SPARSE, 8),
            Family(TRANSFER | SPARSE | VIDEO_DECODE, 1),
            Family(TRANSFER | SPARSE | VIDEO_ENCODE, 1),
            Family(TRANSFER | SPARSE | OPTICAL_FLOW, 1),
        };
    }
}

TEST_SUITE("QueueSelection [vk]") {

TEST_CASE("Discrete NVIDIA table resolves to the dedicated DMA and async-compute families") {
    const auto families = Rtx5070Families();
    const std::vector<VkBool32> present{VK_TRUE, VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_FALSE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    REQUIRE(indices.isComplete());
    CHECK(indices.graphicsFamily.value() == 0);
    //! Graphics can present, so present stays on it -- this is what keeps the swapchain EXCLUSIVE
    CHECK(indices.presentFamily.value() == 0);
    CHECK(indices.computeFamily.value() == 2);
    //! Family 1 is the copy engine. Families 3-5 are equally "dedicated" by capability count;
    //! the flags tie-break sorts the video/optical-flow engines behind it
    CHECK(indices.transferFamily.value() == 1);
}

TEST_CASE("Single-family device puts every role on family 0") {
    const std::vector<VkQueueFamilyProperties> families{
        Family(GRAPHICS | COMPUTE | TRANSFER | SPARSE, 1)
    };
    const std::vector<VkBool32> present{VK_TRUE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    REQUIRE(indices.isComplete());
    CHECK(indices.graphicsFamily.value() == 0);
    CHECK(indices.presentFamily.value() == 0);
    CHECK(indices.computeFamily.value() == 0);
    CHECK(indices.transferFamily.value() == 0);
}

TEST_CASE("AMD-like table picks the transfer-only and compute-only families") {
    const std::vector<VkQueueFamilyProperties> families{
        Family(GRAPHICS | COMPUTE | TRANSFER, 1),
        Family(COMPUTE | TRANSFER, 4),
        Family(TRANSFER, 2),
    };
    const std::vector<VkBool32> present{VK_TRUE, VK_TRUE, VK_FALSE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    REQUIRE(indices.isComplete());
    CHECK(indices.graphicsFamily.value() == 0);
    CHECK(indices.presentFamily.value() == 0);
    CHECK(indices.computeFamily.value() == 1);
    CHECK(indices.transferFamily.value() == 2);
}

TEST_CASE("GRAPHICS or COMPUTE implies transfer capability even without the bit") {
    //! Per spec a graphics- or compute-capable family supports transfers regardless of
    //! VK_QUEUE_TRANSFER_BIT, so a device that never sets the bit must still resolve
    const std::vector<VkQueueFamilyProperties> families{
        Family(GRAPHICS | COMPUTE, 1)
    };
    const std::vector<VkBool32> present{VK_TRUE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    REQUIRE(indices.isComplete());
    CHECK(indices.transferFamily.value() == 0);
}

TEST_CASE("A dedicated transfer family beats a graphics family that also transfers") {
    //! Regression guard on the ranking rule. Ordering on the raw flags value would pick
    //! family 0 here (GRAPHICS|TRANSFER == 5 sorts below TRANSFER|SPARSE == 12) because
    //! GRAPHICS is the numerically cheapest bit -- exactly backwards for a copy queue
    const std::vector<VkQueueFamilyProperties> families{
        Family(GRAPHICS | TRANSFER, 1),
        Family(TRANSFER | SPARSE, 2),
    };
    const std::vector<VkBool32> present{VK_TRUE, VK_FALSE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    CHECK(indices.graphicsFamily.value() == 0);
    CHECK(indices.transferFamily.value() == 1);
}

TEST_CASE("Present falls back to another family when graphics cannot present") {
    const auto families = Rtx5070Families();
    const std::vector<VkBool32> present{VK_FALSE, VK_FALSE, VK_TRUE, VK_FALSE, VK_FALSE, VK_FALSE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    REQUIRE(indices.isComplete());
    CHECK(indices.graphicsFamily.value() == 0);
    //! Split present family: the swapchain falls back to CONCURRENT sharing
    CHECK(indices.presentFamily.value() == 2);
}

TEST_CASE("forceUnified collapses every role onto the graphics family") {
    const auto families = Rtx5070Families();
    const std::vector<VkBool32> present{VK_TRUE, VK_TRUE, VK_TRUE, VK_FALSE, VK_FALSE, VK_FALSE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, true);

    REQUIRE(indices.isComplete());
    CHECK(indices.graphicsFamily.value() == 0);
    CHECK(indices.presentFamily.value() == 0);
    CHECK(indices.computeFamily.value() == 0);
    CHECK(indices.transferFamily.value() == 0);
}

TEST_CASE("forceUnified refuses to fabricate capabilities the graphics family lacks") {
    //! Collapsing is a test knob, not a licence to claim compute on a graphics-only family
    //! or present on a family the surface rejected
    const std::vector<VkQueueFamilyProperties> families{
        Family(GRAPHICS, 1),
        Family(COMPUTE | TRANSFER, 1),
    };
    const std::vector<VkBool32> present{VK_FALSE, VK_TRUE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, true);

    REQUIRE(indices.isComplete());
    CHECK(indices.graphicsFamily.value() == 0);
    //! Transfer is implied by graphics, so this one does collapse
    CHECK(indices.transferFamily.value() == 0);
    //! These two cannot, and must keep their real families
    CHECK(indices.computeFamily.value() == 1);
    CHECK(indices.presentFamily.value() == 1);
}

TEST_CASE("Families reporting zero queues are skipped") {
    const std::vector<VkQueueFamilyProperties> families{
        Family(GRAPHICS | COMPUTE | TRANSFER, 0),
        Family(GRAPHICS | COMPUTE | TRANSFER, 1),
    };
    const std::vector<VkBool32> present{VK_TRUE, VK_TRUE};

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    REQUIRE(indices.isComplete());
    CHECK(indices.graphicsFamily.value() == 1);
    CHECK(indices.transferFamily.value() == 1);
}

TEST_CASE("A device with no present support leaves the selection incomplete") {
    //! FindQueueFamilies passes an all-false table when probing without a surface, and
    //! RateDeviceSuitability rejects on isComplete() -- pin that this stays false
    const auto families = Rtx5070Families();
    const std::vector<VkBool32> present(families.size(), VK_FALSE);

    const auto indices = VkUtil::SelectQueueFamilies(families, present, false);

    CHECK_FALSE(indices.isComplete());
    CHECK_FALSE(indices.presentFamily.has_value());
    //! The rest still resolves -- the compute capability probe depends on it
    CHECK(indices.computeFamily.value() == 2);
}

}

#endif // SHIFT_VULKAN_BACKEND
