#ifndef SHIFT_VKSAMPLER_HPP
#define SHIFT_VKSAMPLER_HPP

#include "VKDevice.hpp"

#include "Graphics/RHI/Common/Sampler.hpp"

namespace Shift::VK {
    class Sampler {
        friend VK::ResourceSet;
        friend VK::ImGuiBackend;
    public:
        //! Initialize a sampler with the RHI desc
        //! \param device The VkDevice
        //! \param desc RHI desc
        //! \return true if successful, false otherwise
        Sampler(const Device* device, const SamplerDescriptor& desc);
        Sampler(const Sampler&)=delete;
        Sampler& operator=(const Sampler&)=delete;

        [[nodiscard]] bool IsValid() const { return m_valid; }

        ~Sampler();
    private:
        //! API SPECIFIC, hence friended.
        //! \return VkSampler
        [[nodiscard]] VkSampler VK_Get() const { return m_sampler; }

        const Device* m_device = nullptr;

        VkSampler m_sampler;
        bool m_valid = false;
    };

    ASSERT_INTERFACE(ISampler, Sampler);
} // Shift::VK

#endif //SHIFT_VKSAMPLER_HPP
