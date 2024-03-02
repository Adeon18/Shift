#include "DescriptorManagement.hpp"

namespace shift::gfx {
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

    bool DescriptorPool::Build() {
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(m_sizes.size());;
        poolInfo.pPoolSizes = m_sizes.data();
        poolInfo.maxSets = m_maxSets;

        m_pool = m_device.CreateDescriptorPool(poolInfo);

        return m_pool != VK_NULL_HANDLE;
    }

    DescriptorPool::~DescriptorPool() {
        m_device.DestroyDescriptorPool(m_pool);
    }

    DescriptorSet::DescriptorSet(const shift::gfx::Device &device): m_device{device} {
    }

    void DescriptorSet::UpdateImage(uint32_t bind, VkImageView view, VkSampler sampler) {
        m_imageInfos.emplace_back(sampler, view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        VkWriteDescriptorSet writeSet{};

        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = m_set;
        writeSet.dstBinding = bind;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &m_imageInfos.back();

        m_writeSets.push_back(writeSet);
    }

    bool DescriptorSet::Allocate(VkDescriptorPool pool, VkDescriptorSetLayout layout) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        m_set = m_device.AllocateDescriptorSet(allocInfo);
        return m_set != VK_NULL_HANDLE;
    }

    void DescriptorSet::ProcessUpdates() {
        vkUpdateDescriptorSets(m_device.Get(), static_cast<uint32_t>(m_writeSets.size()), m_writeSets.data(), 0, nullptr);
        m_writeSets.clear();
        m_imageInfos.clear();
        m_bufferInfos.clear();
    }


    void DescriptorSet::UpdateUBO(uint32_t bind, VkBuffer buffer, uint32_t offset, uint64_t size) {
        m_bufferInfos.emplace_back(buffer, offset, size);

        VkWriteDescriptorSet writeSet{};

        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstBinding = bind;
        writeSet.dstSet = m_set;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeSet.descriptorCount = 1;
        writeSet.pBufferInfo = &m_bufferInfos.back();

        m_writeSets.push_back(writeSet);
    }

    DescriptorSet::~DescriptorSet() {

    }
} // shift::gfx
