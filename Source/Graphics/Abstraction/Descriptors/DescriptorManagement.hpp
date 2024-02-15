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

        VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
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

        VkDescriptorPool m_pool = VK_NULL_HANDLE;
    };

    class DescriptorSet {
    public:
        explicit DescriptorSet(const Device& device);

        //! Add UBO(need to know buffer type) - NEED TO ALLOCATE BEFORE
        template<typename T>
        void UpdateUBO(uint32_t bind, VkBuffer buffer, uint32_t offset);
        //! Add image sampler for read optimal read - NEED TO ALLOCATE BEFORE
        void UpdateImage(uint32_t bind, VkImageView view, VkSampler sampler);

        //! Allocate set
        bool Allocate(VkDescriptorPool pool);

        //! Process updates in descriptor write sets, clears all logged sets after update
        void ProcessUpdates();

        [[nodiscard]] VkDescriptorSet Get() const { return m_set; }
        [[nodiscard]] DescriptorLayout& GetLayout() { return *m_layout; }

        ~DescriptorSet();

        DescriptorSet() = delete;
        DescriptorSet& operator=(const DescriptorSet&)=delete;
        DescriptorSet(const DescriptorSet&) = delete;
    private:
        const Device& m_device;

        std::shared_ptr<DescriptorLayout> m_layout;

        std::vector<VkDescriptorBufferInfo> m_bufferInfos;
        std::vector<VkDescriptorImageInfo> m_imageInfos;
        std::vector<VkWriteDescriptorSet> m_writeSets;
        uint32_t m_maxSets = 1;

        VkDescriptorSet m_set = VK_NULL_HANDLE;
    };

    template<typename T>
    void DescriptorSet::UpdateUBO(uint32_t bind, VkBuffer buffer, uint32_t offset) {
        m_bufferInfos.emplace_back(buffer, offset, sizeof(T));

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
} // sft::gfx

#endif //SHIFT_DESCRIPTORMANAGEMENT_HPP
