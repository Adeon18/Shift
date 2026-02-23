#ifndef SHIFT_VKRESOURCESET_HPP
#define SHIFT_VKRESOURCESET_HPP

#include "VKDevice.hpp"

#include "../Common/ResourceSet.hpp"

namespace Shift::VK {
    class ResourceSet {
    public:
        //! Init the resource set (just fills infos, the api related logic is in the RHI wrapper)
        //! \param device - device lol
        //! \param set - descriptor set
        ResourceSet(const Device* device, VkDescriptorSet set);
        ResourceSet(const ResourceSet&) = delete;
        ResourceSet& operator=(const ResourceSet&) = delete;

        [[nodiscard]] bool IsValid() const { return valid; }

        //! Update UBO at whole buffer size
        //! \param bind
        //! \param InputBuffer
        void UpdateUBO(uint32_t bind, const Buffer& InputBuffer);

        //! Update UBO at custom buffer size and offset(for when you don't want to bind the whole buffer)
        //! \param bind
        //! \param InputBuffer
        //! \param offset
        //! \param size
        void UpdateUBO(uint32_t bind, const Buffer& InputBuffer, uint32_t size, uint32_t offset);

        //! Update Image
        //! \param bind
        //! \param InputTexture
        //! \param arrElement
        void UpdateTexture(uint32_t bind, uint32_t arrElement, const Texture& InputTexture);

        //! Update Sampler
        //! \param bind
        //! \param InputSampler
        void UpdateSampler(uint32_t bind, const Sampler& InputSampler);

        //! Apply the updates, if this is not called after the update functions, none will stick!
        void Apply();

        ~ResourceSet()=default;
    private:
        const Device* m_device;

        // There are needed to keep the stucts "alive before update"
        // TODO: [OPTIMIZATION] Can you remove them?
        std::vector<VkDescriptorBufferInfo> m_bufferInfos;
        std::vector<VkDescriptorImageInfo> m_imageInfos;
        std::vector<VkDescriptorImageInfo> m_samplerInfos;

        std::vector<VkWriteDescriptorSet> m_writeSets;

        VkDescriptorSet m_set = VK_NULL_HANDLE;
        bool valid = false;
    };

    ASSERT_INTERFACE(IResourceSet, ResourceSet);
} // Shift::VK

#endif //SHIFT_VKRESOURCESET_HPP
