#include <vector>
#include <memory>

#include "Graphics/Abstraction/Device/Device.hpp"

#ifndef SHIFT_DESCRIPTORMANAGEMENT_HPP
#define SHIFT_DESCRIPTORMANAGEMENT_HPP

namespace sft::gfx {
    class DescriptorLayout {
    public:
        DescriptorLayout(const Device& device): m_device{device} {}

        //! Add different types of bindings
        void AddUBOBinding(uint32_t bind, VkShaderStageFlags stages);
        void AddSamplerBinding(uint32_t bind, VkShaderStageFlags stages);
        //! Build Layout
        bool Build();

        [[nodiscard]] VkDescriptorSetLayout Get() const { return m_layout; }
        [[nodiscard]] VkDescriptorSetLayout* Ptr() { return &m_layout; }

        ~DescriptorLayout();

        DescriptorLayout() = delete;
        DescriptorLayout(const DescriptorLayout&) = delete;
        DescriptorLayout& operator=(const DescriptorLayout&) = delete;
    private:
        const Device& m_device;

        std::vector<VkDescriptorSetLayoutBinding> m_bindings;

        VkDescriptorSetLayout m_layout;
    };

    class DescriptorPool {
    public:
        DescriptorPool(const Device& device): m_device{device} {}

        //! Add different types of sizes
        void AddUBOSize(uint32_t limit);
        void AddSamplerSize(uint32_t limit);
        void SetMaxSets(uint32_t limit);

        //! Build Pool
        bool Build();

        [[nodiscard]] VkDescriptorPool Get() const { return m_pool; }

        ~DescriptorPool();

        DescriptorPool() = delete;
        DescriptorPool(const DescriptorPool&) = delete;
        DescriptorPool& operator=(const DescriptorPool&) = delete;
    private:
        const Device& m_device;

        std::vector<VkDescriptorPoolSize> m_sizes;
        uint32_t m_maxSets = 1;

        VkDescriptorPool m_pool;
    };

    class DescriptorSet {
    public:
        DescriptorSet(const Device& device);

        //! Add UBO(need to know buffer type) - NEED TO ALLOCATE BEFORE
        template<typename T>
        void AddUBO(VkBuffer buffer, uint32_t offset, uint32_t bind, VkShaderStageFlags stages);
        //! Add image sampler for read optimal read - NEED TO ALLOCATE BEFORE
        void AddImage(VkImageView view, VkSampler sampler, uint32_t bind, VkShaderStageFlags stages);

        //! Allocate set TODO: SET dst set
        bool Build(VkDescriptorPool pool);

        [[nodiscard]] VkDescriptorSet Get() const { return m_set; }
        [[nodiscard]] VkDescriptorSetLayout GetLayout() const { return m_layout->Get(); }
        [[nodiscard]] VkDescriptorSetLayout* GetLayoutPtr() { return m_layout->Ptr(); }

        ~DescriptorSet();

        DescriptorSet() = delete;
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;
    private:
        const Device& m_device;

        std::unique_ptr<DescriptorLayout> m_layout;

        std::vector<VkDescriptorBufferInfo> m_bufferInfos;
        std::vector<VkDescriptorImageInfo> m_imageInfos;
        std::vector<VkWriteDescriptorSet> m_writeSets;
        uint32_t m_maxSets = 1;

        VkDescriptorSet m_set = VK_NULL_HANDLE;
    };

    template<typename T>
    void DescriptorSet::AddUBO(VkBuffer buffer, uint32_t offset, uint32_t bind, VkShaderStageFlags stages) {
        m_layout->AddUBOBinding(bind, stages);

        m_bufferInfos.emplace_back(buffer, offset, sizeof(T));

        VkWriteDescriptorSet writeSet{};

        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstBinding = bind;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeSet.descriptorCount = 1;
        writeSet.pBufferInfo = &m_bufferInfos.back();

        m_writeSets.push_back(writeSet);
    }
} // sft::gfx

#endif //SHIFT_DESCRIPTORMANAGEMENT_HPP
