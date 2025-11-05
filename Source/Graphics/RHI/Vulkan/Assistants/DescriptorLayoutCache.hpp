//
// Created by otrush on 10/27/2025.
//

#ifndef SHIFT_DESCRIPTORLAYOUTCACHE_HP
#define SHIFT_DESCRIPTORLAYOUTCACHE_HP

#include <vector>

#include "Graphics/RHI/Vulkan/VKDevice.hpp"

namespace Shift::VK {
    class DescriptorLayoutCache {
    public:
        void Init(const Device* device);
        void Destroy();

        //! THe data conversion to this input format lies on the SRHI
        VkDescriptorSetLayout CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo& info);

        //! Layout info stucture
        struct DescriptorLayoutInfo {
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            bool operator==(const DescriptorLayoutInfo& other) const;

            size_t Hash() const;
        };

    private:
        const Device* m_device;

        struct DescriptorLayoutHash {
            std::size_t operator()(const DescriptorLayoutInfo& k) const{
                return k.Hash();
            }
        };

        std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> layoutCache;
    };

} // Shift::VK

#endif //SHIFT_DESCRIPTORLAYOUTCACHE_HP