#ifndef SHIFT_PASSPRIMITIVES_HPP
#define SHIFT_PASSPRIMITIVES_HPP

#include <vulkan/vulkan.h>
#include <vector>

namespace sft {
    namespace gfx {
        struct Attachment {
        public:
            enum Type {
                Color,
                Depth,
                Swapchain
            };

            //! Set the description and reference based on type, bind and format
            //! Leaves some fields to default values, expected to be either ignored or rewritten to externally
            Attachment(VkFormat format, Type type, uint32_t bind);

            VkAttachmentDescription description{};
            VkAttachmentReference reference{};

            Type type;
        };

        struct Subpass {

            void AddDependency(const Attachment& att, uint32_t srcSubpass = VK_SUBPASS_EXTERNAL, uint32_t dstSubpass = 0);
            //! Builds the subpass description, has to be called before creating the renderpass!
            void BuildDescription();

            std::vector<VkAttachmentReference> colorAttachmentRefs;
            VkAttachmentReference depthAttachmentRef;
            std::vector<VkSubpassDependency> dependencies;

            VkSubpassDescription description{};
        };
    } // gfx
} // sft

#endif //SHIFT_PASSPRIMITIVES_HPP
