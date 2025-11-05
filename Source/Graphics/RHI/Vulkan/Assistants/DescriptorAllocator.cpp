#include "DescriptorAllocator.hpp"


namespace Shift::VK {
    bool DescriptorAllocator::Init(const Device* device, uint32_t initialSets) {
        m_device = device;

        for (auto r: DEFAULT_SIZE_CONFIG) {
            m_sizeRatios.push_back(r);
        }

        VkDescriptorPool newPool = CreatePool(initialSets, m_sizeRatios);

        m_setsPerPool = initialSets * 2u;

        m_readyPools.push_back(newPool);

        return true;
    }

    void DescriptorAllocator::Destroy() {
        for (auto p: m_readyPools) {
            m_device->DestroyDescriptorPool(p);
        }
        m_readyPools.clear();

        for (auto p: m_fullPools) {
            m_device->DestroyDescriptorPool(p);
        }
        m_fullPools.clear();
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

    VkDescriptorPool DescriptorAllocator::CreatePool(uint32_t setCount, std::span<PoolSizeRatio> poolRatios) {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (PoolSizeRatio ratio : poolRatios) {
            poolSizes.push_back(VkDescriptorPoolSize{
                    .type = ratio.type,
                    .descriptorCount = uint32_t(ratio.ratio * setCount)
            });
        }

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = 0;
        pool_info.maxSets = setCount;
        pool_info.poolSizeCount = (uint32_t)poolSizes.size();
        pool_info.pPoolSizes = poolSizes.data();

        VkDescriptorPool newPool = m_device->CreateDescriptorPool(pool_info);
        return newPool;
    }

    VkDescriptorSet DescriptorAllocator::Allocate(VkDescriptorSetLayout layout) {
        VkDescriptorPool poolToUse = GetPool();

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = poolToUse;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkResult result;
        VkDescriptorSet ds = m_device->AllocateDescriptorSet(allocInfo, &result);

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