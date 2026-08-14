#include "DescriptorAllocator.hpp"

#include "Config/EngineConfig.hpp"


namespace Shift::VK {
    DescriptorAllocator::DescriptorAllocator(const Device* device, uint32_t initialSets) {
        m_device = device;

        for (auto r: DEFAULT_SIZE_CONFIG) {
            m_sizeRatios.push_back(r);
        }

        VkDescriptorPool newPool = CreatePool(initialSets, m_sizeRatios, GENERAL_POOL_FLAGS);

        m_setsPerPool = initialSets * 2u;

        m_readyPools.push_back(newPool);

        std::vector<PoolSizeRatio> imguiRatios;
        for (auto r: IMGUI_POOL_RATIOS) {
            imguiRatios.push_back(r);
        }
        //! Extra beefy pool for imgui
        m_imguiPool = CreatePool(500, imguiRatios, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

        std::vector<PoolSizeRatio> bindlessPoolRatios {
            PoolSizeRatio{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, Conf::MAX_BINDLESS_IMAGES},
            PoolSizeRatio{VK_DESCRIPTOR_TYPE_SAMPLER, Conf::MAX_BINDLESS_SAMPLERS}
        };
        //! We can have multiple bindless sets
        m_bindlessPool = CreatePool(Conf::MAX_BINDLESS_SETS, bindlessPoolRatios, VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT);
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
        m_device->DestroyDescriptorPool(m_bindlessPool);
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
            newPool = CreatePool(m_setsPerPool, m_sizeRatios, GENERAL_POOL_FLAGS);

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

    VkDescriptorSet DescriptorAllocator::Allocate(VkDescriptorSetLayout layout, bool fromBindlessPool) {

        VkDescriptorPool poolToUse = fromBindlessPool ? m_bindlessPool : GetPool();

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = poolToUse;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        //! We dropped variable count here as variable count can have only one binding in sset and we want our set to have multiple
        VkResult result;
        VkDescriptorSet ds = m_device->AllocateDescriptorSet(allocInfo, &result);

        //! The bindless pool is fixed-size and not recycled, so there is no second attempt
        if (fromBindlessPool) {
            if (ds == VK_NULL_HANDLE) {
                Log(Error, "Failed to allocate a descriptor set from the bindless pool. It is sized for {} set(s)", Conf::MAX_BINDLESS_SETS);
            }
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