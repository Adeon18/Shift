#include "VKSampler.hpp"

#include "Utility/Vulkan/VKDebugUtils.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"

namespace Shift::VK {
    Sampler::Sampler(const Device* device, const Shift::SamplerDescriptor &desc): m_device(device) {
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

        m_valid = VkNullCheck(m_sampler);

        if (m_valid) {
            Util::SetDebugName(m_device->Get(), VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(m_sampler), desc.name.c_str());
        }
    }

    Sampler::~Sampler() {
        m_device->DestroyImageSampler(m_sampler);
    }
} // Shift::VK