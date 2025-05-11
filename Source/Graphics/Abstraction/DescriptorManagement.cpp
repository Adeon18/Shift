#include "DescriptorManagement.hpp"

namespace Shift::gfx {
    void DescriptorLayout::AddUBOBinding(uint32_t bind, VkShaderStageFlags stages) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = bind;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = stages;

        m_bindings.push_back(uboLayoutBinding);
    }

    void DescriptorLayout::AddSamplerBinding(uint32_t bind, VkShaderStageFlags stages) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = bind;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = stages;

        m_bindings.push_back(uboLayoutBinding);
    }

    bool DescriptorLayout::Build() {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());
        layoutInfo.pBindings = m_bindings.data();

        m_layout = m_device.CreateDescriptorSetLayout(layoutInfo);
        return m_layout != VK_NULL_HANDLE;
    }

    DescriptorLayout::~DescriptorLayout() {
        m_device.DestroyDescriptorSetLayout(m_layout);
    }

    void DescriptorPool::AddUBOSize(uint32_t limit) {
        m_sizes.emplace_back(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, limit);
    }

    void DescriptorPool::AddSamplerSize(uint32_t limit) {
        m_sizes.emplace_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, limit);
    }

    void DescriptorPool::SetMaxSets(uint32_t limit) { m_maxSets = limit; }

    bool DescriptorPool::Build(VkDescriptorPoolCreateFlags flags) {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = flags;
        poolInfo.poolSizeCount = static_cast<uint32_t>(m_sizes.size());;
        poolInfo.pPoolSizes = m_sizes.data();
        poolInfo.maxSets = m_maxSets;

        m_pool = m_device.CreateDescriptorPool(poolInfo);

        return m_pool != VK_NULL_HANDLE;
    }

    DescriptorPool::~DescriptorPool() {
        m_device.DestroyDescriptorPool(m_pool);
    }
} // shift::gfx
