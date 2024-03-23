//
// Created by otrush on 3/23/2024.
//

#ifndef SHIFT_SAMPLERMANAGER_HPP
#define SHIFT_SAMPLERMANAGER_HPP

#include "Graphics/Abstraction/Device/Device.hpp"

#include "Utility/Vulkan/InfoUtil.hpp"

namespace shift::gfx {
    //! A RAII storage for samplers
    class SamplerManager {
    public:
        SamplerManager(const Device& device);

        [[nodiscard]] VkSampler GetPointSampler() const { return m_pointSampler; }
        [[nodiscard]] VkSampler GetLinearSampler() const { return m_linearSampler; }

        ~SamplerManager();

        SamplerManager() = delete;
        SamplerManager(const SamplerManager&) = delete;
        SamplerManager& operator=(const SamplerManager&) = delete;
    private:
        const Device& m_device;

        VkSampler m_pointSampler;
        VkSampler m_linearSampler;
    };
}

#endif //SHIFT_SAMPLERMANAGER_HPP
