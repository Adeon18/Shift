#include "DescriptorAllocator.hpp"

#include "Config/EngineConfig.hpp"


namespace Shift::VK {
    DescriptorAllocator::DescriptorAllocator(const Device* device, uint32_t initialSets) {
        m_device = device;

        for (auto r: DEFAULT_SIZE_CONFIG) {
            m_sizeRatios.push_back(r);
        }

        VkDescriptorPool newPool = CreatePool(initialSets, m_sizeRatios);

        m_setsPerPool = initialSets * 2u;

        m_readyPools.push_back(newPool);

        std::vector<PoolSizeRatio> imguiRatios;
        for (auto r: IMGUI_POOL_RATIOS) {
            imguiRatios.push_back(r);
        }
        //! Extra beefy pool for imgui
        m_imguiPool = CreatePool(500, imguiRatios, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

        std::vector<PoolSizeRatio> bindlessPoolRatios {PoolSizeRatio{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, Conf::MAX_BINDLESS_IMAGES}};
        m_bindlessTexturePool = CreatePool(1, bindlessPoolRatios, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
    }

    DescriptorAllocator::~DescriptorAllocator() {
        for (auto p: m_readyPools) {
            m_device->DestroyDescriptorPool(p);
        }
        m_readyPools.clear();

        for (auto p: m_fullPools) {
            m_device->DestroyDescriptorPool(p);
        }
        m_fullPools.clear();

        m_device->DestroyDescriptorPool(m_imguiPool);
        m_device->DestroyDescriptorPool(m_bindlessTexturePool);
    }

    void DescriptorAllocator::Clear() {
        for (auto p: m_readyPools) {
            m_device->ResetDescriptorPool(p);
        }
        for (auto p: m_fullPools) {
            m_device->DestroyDescriptorPool(p);
            m_readyPools.push_back(p);
        }
        m_fullPools.clear();
    }

    VkDescriptorPool DescriptorAllocator::GetPool() {
        VkDescriptorPool newPool;
        if (!m_readyPools.empty()) {
            newPool = m_readyPools.back();
            m_readyPools.pop_back();
        }
        else {
            //! Need to create a new pool
            newPool = CreatePool(m_setsPerPool, m_sizeRatios);

            m_setsPerPool = m_setsPerPool * 1.5;
            if (m_setsPerPool > SET_LIMIT_PER_POOL) {
                m_setsPerPool = SET_LIMIT_PER_POOL;
            }
        }

        return newPool;
    }

    VkDescriptorPool DescriptorAllocator::CreatePool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios, VkDescriptorPoolCreateFlags flags) {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (PoolSizeRatio ratio : poolRatios) {
            poolSizes.push_back(VkDescriptorPoolSize{
                    .type = ratio.type,
                    .descriptorCount = uint32_t(ratio.ratio * setCount)
            });
        }

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = flags;
        pool_info.maxSets = setCount;
        pool_info.poolSizeCount = (uint32_t)poolSizes.size();
        pool_info.pPoolSizes = poolSizes.data();

        VkDescriptorPool newPool = m_device->CreateDescriptorPool(pool_info);
        return newPool;
    }

    VkDescriptorSet DescriptorAllocator::Allocate(VkDescriptorSetLayout layout, uint32_t bindlessCount, EBindingType bindlessType) {

        bool isBindless = bindlessCount > 0;

        VkDescriptorPool poolToUse = VK_NULL_HANDLE;
        if (isBindless) {
            switch (bindlessType) {
                case EBindingType::SampledImage:
                default:
                    poolToUse = m_bindlessTexturePool;
            }
        } else {
            poolToUse = GetPool();
        }

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = poolToUse;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        //! Bindless handling
        VkDescriptorSetVariableDescriptorCountAllocateInfo countInfo{};
        if (isBindless) {
            countInfo.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
            countInfo.descriptorSetCount = 1;
            countInfo.pDescriptorCounts = &bindlessCount;
            allocInfo.pNext = &countInfo;
        }

        VkResult result;
        VkDescriptorSet ds = m_device->AllocateDescriptorSet(allocInfo, &result);

        //! Early exit at bindless as we do not want to manage the pool structs
        if (isBindless) {
            return ds;
        }

        //! Allocation failed. Try again but if not then we fucked up
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
            m_fullPools.push_back(poolToUse);

            poolToUse = GetPool();
            allocInfo.descriptorPool = poolToUse;

            ds = m_device->AllocateDescriptorSet(allocInfo, &result);

            if ( VkCheck(result) ) {
                Log(Error, "Error allocating a descriptor set from the descriptor pool");
                ds = VK_NULL_HANDLE;
                return ds;
            }
        }

        m_readyPools.push_back(poolToUse);
        return ds;
    }
} // SHift::VK