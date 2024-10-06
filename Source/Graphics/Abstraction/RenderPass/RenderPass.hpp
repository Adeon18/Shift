#ifndef SHIFT_RENDERPASS_HPP
#define SHIFT_RENDERPASS_HPP

#include "PassPrimitives.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"

namespace Shift {
    namespace gfx {
        class RenderPass {
        public:
            RenderPass(const Device& device): m_device{device} {}
            RenderPass() = delete;
            RenderPass(const RenderPass& device) = delete;
            RenderPass& operator=(const Device& device) = delete;

            void AddAttachment(const Attachment& attachment);
            void AddSubpass(const Subpass& pass);

            //! Gather all data from stored data and build the pass
            [[nodiscard]] bool BuildPass();

            [[nodiscard]] VkRenderPass Get() const { return m_pass; }

            ~RenderPass();
        private:
            const Device& m_device;

            std::vector<Subpass> m_subpassData;
            std::vector<Attachment> m_attachmentData;

            VkRenderPass m_pass;
        };
    } // gfx
} // shift

#endif //SHIFT_RENDERPASS_HPP
