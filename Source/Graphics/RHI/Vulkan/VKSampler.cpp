#include "VKSampler.hpp"

#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    void Sampler::Init(const Device* device, const Shift::SamplerDescriptor &desc) {
        m_device = device;
        m_sampler = m_device->CreateImageSampler(
                Util::CreateSamplerInfo(
                        Util::ShiftToVKFilterMode(desc.minFilter),
                        Util::ShiftToVKFilterMode(desc.magFilter),
                        Util::ShiftToVKMipMapMode(desc.mipFilter),
                        Util::ShiftToVKSamplerAddressMode(desc.addressModeU),
                        Util::ShiftToVKSamplerAddressMode(desc.addressModeV),
                        Util::ShiftToVKSamplerAddressMode(desc.addressModeW),
                        desc.unnormalizedCoordinates,
                        desc.compareEnable,
                        Util::ShiftToVKCompareOperation(desc.compareFunction),
                        desc.mipLodBias,
                        desc.minLod,
                        desc.maxLod
                )
        );

        valid = VkNullCheck(m_sampler);
    }

    void Sampler::Destroy() {
        m_device->DestroyImageSampler(m_sampler);
    }
} // Shift::VK