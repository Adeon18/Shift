#ifndef SHIFT_DESCRIPTORALLOCATOR_HPP
#define SHIFT_DESCRIPTORALLOCATOR_HPP

#include <span>
#include <vector>

#include "Graphics/RHI/Vulkan/VKDevice.hpp"

namespace Shift::VK {
    //! A growable allocator for descriptor sets
    class DescriptorAllocator {
    public:
        struct PoolSizeRatio {
            VkDescriptorType type;
            float ratio;
        };

        const std::vector<PoolSizeRatio> DEFAULT_SIZE_CONFIG =
        {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
        };

        static constexpr uint32_t SET_LIMIT_PER_POOL = 4096u;

        //! Initialize the Descriptor allocator, which will create descriptor pools under the hood
        //! \param device
        //! \param initialSets The base amount of pool descriptor size
        //! \param poolRatios The descriptor type ration setup from PoolSizeRatio
        //! \return
        bool Init(const Device* device, uint32_t initialSets = 4);

        //! Clears all pools
        void Clear();

        //! Destroys all pools
        void Destroy();

        //! Get a free pool
        //! TODO: [FEATURE] Be able to get pools by their flag?
        // \return The descriptor pool
        VkDescriptorPool GetPool();

        /// Allocate a descriptor set
        /// \param layout descriptor layout
        /// \return allocated set, VK_NULL_HANDLE if there was an error
        VkDescriptorSet Allocate(VkDescriptorSetLayout layout);
    private:
        //! Create a pool
        //! \param device
        //! \param setCount The set count from which the number of pool type objects will be determined
        //! \param poolRatios pool types themselves
        //! \return
        VkDescriptorPool CreatePool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios);
    private:
        const Device* m_device;

        std::vector<PoolSizeRatio> m_sizeRatios;
        std::vector<VkDescriptorPool> m_fullPools;
        std::vector<VkDescriptorPool> m_readyPools;

        uint32_t m_setsPerPool = 1u;
    };
} // Shift::VK

#endif //SHIFT_DESCRIPTORALLOCATOR_HPP
