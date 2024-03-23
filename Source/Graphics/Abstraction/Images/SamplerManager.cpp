//
// Created by otrush on 3/23/2024.
//

#include "SamplerManager.hpp"

namespace shift::gfx {
    SamplerManager::SamplerManager(const shift::gfx::Device &device): m_device{device} {
        m_pointSampler = m_device.CreateImageSampler(info::CreateSamplerInfo(VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT));
        m_linearSampler = m_device.CreateImageSampler(info::CreateSamplerInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    }

    SamplerManager::~SamplerManager() {
        m_device.DestroyImageSampler(m_pointSampler);
        m_device.DestroyImageSampler(m_linearSampler);
    }
}