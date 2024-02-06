#include "RenderPass.hpp"

namespace sft {
    namespace gfx {
        void RenderPass::AddAttachment(const sft::gfx::Attachment &attachment) {
            m_attachmentData.push_back(attachment);
        }

        void RenderPass::AddSubpass(const sft::gfx::Subpass &pass) {
            m_subpassData.push_back(pass);
        }

        bool RenderPass::BuildPass() {
            std::vector<VkAttachmentDescription> attachments(m_attachmentData.size());
            std::vector<VkSubpassDependency> dependencies;
            dependencies.reserve(m_subpassData.size());
            std::vector<VkSubpassDescription> subpassDescriptions(m_subpassData.size());

            for (uint32_t i = 0; i < m_attachmentData.size(); ++i) {
                attachments[i] = std::move(m_attachmentData[i].description);
            }

            for (uint32_t i = 0; i < m_subpassData.size(); ++i) {
                for (uint32_t j = 0; j < m_subpassData[i].dependencies.size(); ++j) {
                    dependencies.push_back(std::move(m_subpassData[i].dependencies[j]));
                }
                subpassDescriptions[i] = std::move(m_subpassData[i].description);
            }

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
            renderPassInfo.pSubpasses = subpassDescriptions.data();
            renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies = dependencies.data();

            m_pass = m_device.CreateRenderPass(renderPassInfo);

            return m_pass != VK_NULL_HANDLE;
        }

        RenderPass::~RenderPass() {
            m_device.DestroyRenderPass(m_pass);
        }
    } // gfx
} // sft