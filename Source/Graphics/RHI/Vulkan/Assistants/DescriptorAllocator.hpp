#ifndef SHIFT_DESCRIPTORALLOCATOR_HPP
#define SHIFT_DESCRIPTORALLOCATOR_HPP

#include <span>
#include <vector>

#include "Graphics/RHI/Common/Pipeline.hpp"
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
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0.5f },
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

        const std::vector<PoolSizeRatio> IMGUI_POOL_RATIOS = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1.0f }
        };

        static constexpr uint32_t SET_LIMIT_PER_POOL = 4096u;

        //! We have minimal pools and sets so we can allow this
        static constexpr VkDescriptorPoolCreateFlags GENERAL_POOL_FLAGS = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

        //! Initialize the Descriptor allocator, which will create descriptor pools under the hood
        //! \param device
        //! \param initialSets The base amount of pool descriptor size
        //! \param poolRatios The descriptor type ration setup from PoolSizeRatio
        DescriptorAllocator(const Device* device, uint32_t initialSets = 4);
        DescriptorAllocator(const DescriptorAllocator&) = delete;
        DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

        //! Clears all pools
        void Clear();

        //! Destroys all pools
        ~DescriptorAllocator();

        //! Get a free pool
        //! TODO: [FEATURE] Be able to get pools by their flag?
        // \return The descriptor pool
        VkDescriptorPool GetPool();

        [[nodiscard]] VkDescriptorPool GetImGuiPool() const { return m_imguiPool; }
        [[nodiscard]] VkDescriptorPool GetBindlessPool() const { return m_bindlessPool; }

        /// Allocate a descriptor set
        /// \param layout descriptor layout
        /// \param fromBindlessPool Take it from the UPDATE_AFTER_BIND pool sized for the global set's arrays, rather than from the growable general pools
        /// \return allocated set, VK_NULL_HANDLE if there was an error
        VkDescriptorSet Allocate(VkDescriptorSetLayout layout, bool fromBindlessPool = false);
    private:
        //! Create a pool
        //! \param device
        //! \param setCount The set count from which the number of pool type objects will be determined
        //! \param poolRatios pool types themselves
        //! \return
        VkDescriptorPool CreatePool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios, VkDescriptorPoolCreateFlags flags = 0);
    private:
        const Device* m_device;

        std::vector<PoolSizeRatio> m_sizeRatios;
        std::vector<VkDescriptorPool> m_fullPools;
        std::vector<VkDescriptorPool> m_readyPools;

        //! Created with VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
        VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
        //! Sized for the global set's arrays (images and samplers), UPDATE_AFTER_BIND, MAX_BINDLESS_SETS sets
        VkDescriptorPool m_bindlessPool = VK_NULL_HANDLE;

        uint32_t m_setsPerPool = 1u;
    };
} // Shift::VK

#endif //SHIFT_DESCRIPTORALLOCATOR_HPP
