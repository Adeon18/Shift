#include "PassPrimitives.hpp"

namespace shift {
    namespace gfx {
        Attachment::Attachment(VkFormat format, Type tp, uint32_t bind): type{tp} {
            description.format = format;
            description.samples = VK_SAMPLE_COUNT_1_BIT;
            description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            switch (type) {
                case Type::Color:
                    description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    break;
                case Type::Depth:
                    description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    break;
                case Type::Swapchain:
                    description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                    break;
            }

            reference.attachment = bind;
            switch (type) {
                case Type::Depth:
                    reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    break;
                default:
                    reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    break;
            }
        }

        void Subpass::AddDependency(const Attachment &att, uint32_t srcSubpass, uint32_t dstSubpass) {
            VkSubpassDependency dep{};
            dep.srcSubpass = srcSubpass;
            dep.dstSubpass = dstSubpass;

            // These are default values for depth, color and swapchain, public so you can change them
            switch (att.type) {
                case Attachment::Type::Color:
                    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    dep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                    dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
                case Attachment::Type::Depth:
                    dep.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    dep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    dep.srcAccessMask = 0;
                    dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    dep.dependencyFlags = 0;
                    break;
                case Attachment::Type::Swapchain:
                    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    dep.srcAccessMask = 0;
                    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    dep.dependencyFlags = 0;
                    break;
            }

            dependencies.emplace_back(dep);

            switch (att.type) {
                case Attachment::Type::Color:
                    colorAttachmentRefs.emplace_back(att.reference);
                case Attachment::Type::Depth:
                    depthAttachmentRef = att.reference;
                    break;
                case Attachment::Type::Swapchain:
                    colorAttachmentRefs.emplace_back(att.reference);
                    break;
            }
        }

        void Subpass::BuildDescription() {
            description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            description.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
            description.pColorAttachments = colorAttachmentRefs.data();
            description.pDepthStencilAttachment = &depthAttachmentRef;
        }
    } // gfx
} // shift
