//! Backend-specific tests: these pin the Shift to Vulkan conversion tables, so they compile
//! only in a Vulkan-backend build. The agnostic suites stay backend-neutral: under a future
//! DX12 build the rest of shift_tests runs unchanged and this file simply drops out.
//! Suite tag: [vk] (filter with: shift_tests -ts=*[vk]*).
#ifdef SHIFT_VULKAN_BACKEND

#include <doctest/doctest.h>

#include "Utility/Vulkan/VKUtilRHI.hpp"

using namespace Shift;
namespace VkUtil = Shift::VK::Util;

TEST_SUITE("VKUtilRHI [vk]") {

TEST_CASE("EPipelineStageFlags is bit-identical to VkPipelineStageFlagBits2") {
    //! The F-SYNC2 realignment made the agnostic enum 1:1 with sync2 so the conversion is a
    //! lossless cast

    //! sync1-era bits (0..16)
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::TopOfPipeBit) == VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::VertexShaderBit) == VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::FragmentShaderBit) == VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::ColorAttachmentOutputBit) == VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::ComputeShaderBit) == VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::AllTransferBit) == VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::BottomOfPipeBit) == VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::AllCommandsBit) == VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);

    //! sync2-only bits (32..38): the ones the F-SYNC2 realignment moved
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::CopyBit) == VK_PIPELINE_STAGE_2_COPY_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::ResolveBit) == VK_PIPELINE_STAGE_2_RESOLVE_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::BlitBit) == VK_PIPELINE_STAGE_2_BLIT_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::ClearBit) == VK_PIPELINE_STAGE_2_CLEAR_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::IndexInputBit) == VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::VertexAttributeInputBit) == VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT);
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(EPipelineStageFlags::PreRasterizationShadersBit) == VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT);

    //! Composed masks survive the cast
    const auto composed = EPipelineStageFlags::ColorAttachmentOutputBit | EPipelineStageFlags::CopyBit;
    CHECK(VkUtil::ShiftToVKPipelineStageFlags2(composed)
          == (VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COPY_BIT));
}

TEST_CASE("Stage flags round-trip through both converters") {
    const EPipelineStageFlags samples[] = {
        EPipelineStageFlags::NoneBit,
        EPipelineStageFlags::TopOfPipeBit,
        EPipelineStageFlags::FragmentShaderBit,
        EPipelineStageFlags::AllTransferBit,
        EPipelineStageFlags::PreRasterizationShadersBit,
        EPipelineStageFlags::ColorAttachmentOutputBit | EPipelineStageFlags::BlitBit,
    };

    for (const auto flags : samples) {
        CHECK(VkUtil::VKToShiftPipelineStageFlags2(VkUtil::ShiftToVKPipelineStageFlags2(flags)) == flags);
    }
}

TEST_CASE("EResourceLayout maps to the matching VkImageLayout") {
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::Undefined) == VK_IMAGE_LAYOUT_UNDEFINED);
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::General) == VK_IMAGE_LAYOUT_GENERAL);
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::ColorAttachmentOptimal) == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::DepthStencilAttachmentOptimal) == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::ShaderReadOnlyOptimal) == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::TransferSrcOptimal) == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::TransferDstOptimal) == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //! The non-contiguous KHR value — the one a naive renumbering would break
    CHECK(VkUtil::ShiftToVKResourceLayout(EResourceLayout::Present) == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

TEST_CASE("ETextureFormat maps to the matching VkFormat") {
    CHECK(VkUtil::ShiftToVKTextureFormat(ETextureFormat::UNDEFINED) == VK_FORMAT_UNDEFINED);
    CHECK(VkUtil::ShiftToVKTextureFormat(ETextureFormat::R8G8B8A8_SRGB) == VK_FORMAT_R8G8B8A8_SRGB);
    CHECK(VkUtil::ShiftToVKTextureFormat(ETextureFormat::B8G8R8A8_SRGB) == VK_FORMAT_B8G8R8A8_SRGB);
    CHECK(VkUtil::ShiftToVKTextureFormat(ETextureFormat::D32_SFLOAT) == VK_FORMAT_D32_SFLOAT);
    CHECK(VkUtil::ShiftToVKTextureFormat(ETextureFormat::D24_UNORM_S8_UINT) == VK_FORMAT_D24_UNORM_S8_UINT);
    CHECK(VkUtil::ShiftToVKTextureFormat(ETextureFormat::BC7_SRGB_BLOCK) == VK_FORMAT_BC7_SRGB_BLOCK);
    //! Last enumerator: catches an off-by-one anywhere in the table
    CHECK(VkUtil::ShiftToVKTextureFormat(ETextureFormat::ASTC_12x12_SRGB_BLOCK) == VK_FORMAT_ASTC_12x12_SRGB_BLOCK);

    //! And back
    CHECK(VkUtil::VKToShiftTextureFormat(VK_FORMAT_B8G8R8A8_SRGB) == ETextureFormat::B8G8R8A8_SRGB);
    CHECK(VkUtil::VKToShiftTextureFormat(VK_FORMAT_D32_SFLOAT) == ETextureFormat::D32_SFLOAT);
}

TEST_CASE("ETextureAspect maps to the matching VkImageAspectFlags") {
    CHECK(VkUtil::ShiftToVKTextureAspect(ETextureAspect::Color) == VK_IMAGE_ASPECT_COLOR_BIT);
    CHECK(VkUtil::ShiftToVKTextureAspect(ETextureAspect::Depth) == VK_IMAGE_ASPECT_DEPTH_BIT);
    CHECK(VkUtil::ShiftToVKTextureAspect(ETextureAspect::Stencil) == VK_IMAGE_ASPECT_STENCIL_BIT);
}

}

#endif // SHIFT_VULKAN_BACKEND
