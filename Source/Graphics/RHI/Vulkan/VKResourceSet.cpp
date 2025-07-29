#include "VKResourceSet.hpp"

#include "VKBuffer.hpp"
#include "VKTexture.hpp"
#include "VKSampler.hpp"

#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    void ResourceSet::Init(const Device *device) {
        m_device = device;

        const uint32_t DEFAULT_INFO_SIZE = 16;

        m_imageInfos.reserve(DEFAULT_INFO_SIZE);
        m_bufferInfos.reserve(DEFAULT_INFO_SIZE);
        m_samplerInfos.reserve(DEFAULT_INFO_SIZE);
    }

    void ResourceSet::UpdateUBO(uint32_t bind, const VK::Buffer &InputBuffer) {\
        UpdateUBO(bind, InputBuffer, InputBuffer.GetSize(), 0);
    }

    void ResourceSet::UpdateUBO(uint32_t bind, const VK::Buffer &InputBuffer, uint32_t size, uint32_t offset) {
        m_bufferInfos.emplace_back(InputBuffer.VK_Get(), offset, size);

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

    void ResourceSet::UpdateTexture(uint32_t bind, const VK::Texture &InputTexture) {
        m_imageInfos.emplace_back(VK_NULL_HANDLE, InputTexture.GetView(), Util::ShiftToVKResourceLayout(InputTexture.GetResourceLayout()));

        VkWriteDescriptorSet writeSet{};

        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = m_set;
        writeSet.dstBinding = bind;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &m_imageInfos.back();

        m_writeSets.push_back(writeSet);
    }

    void ResourceSet::UpdateSampler(uint32_t bind, const VK::Sampler &InputSampler) {
        // TODO [SANITY_CHECK] @gronk is this correct?
        m_samplerInfos.emplace_back(InputSampler.VK_Get());

        VkWriteDescriptorSet writeSet{};

        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = m_set;
        writeSet.dstBinding = bind;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &m_imageInfos.back();

        m_writeSets.push_back(writeSet);
    }

    void ResourceSet::Apply() {
        vkUpdateDescriptorSets(m_device->Get(), static_cast<uint32_t>(m_writeSets.size()), m_writeSets.data(), 0, nullptr);
        m_writeSets.clear();
        m_imageInfos.clear();
        m_bufferInfos.clear();
        m_samplerInfos.clear();
    }
} // Shift::VK