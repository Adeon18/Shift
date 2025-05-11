#ifndef SHIFT_VKSAMPLER_HPP
#define SHIFT_VKSAMPLER_HPP

#include "VKDevice.hpp"

#include "Graphics/RHI/Sampler.hpp"

namespace Shift::VK {
    class Sampler {
        friend VK::ResourceSet;
    public:
        Sampler()=default;
        Sampler(const Sampler&) = delete;
        Sampler& operator=(const Sampler&) = delete;

        //! Initialize a sampler with the RHI desc
        //! \param device The VkDevice
        //! \param desc RHI desc
        //! \return true if successful, false otherwise
        [[nodiscard]] bool Init(const Device* device, const SamplerDescriptor& desc);

        void Destroy();
        ~Sampler() = default;
    private:
        //! API SPECIFIC, DO NOT USE UNLESS NESSESARY IN RHI SPECIFIC CODE
        //! \return VkPipeline
        [[nodiscard]] VkSampler VK_Get() const { return m_sampler; }
    private:
        const Device* m_device = nullptr;

        VkSampler m_sampler;
    };

    ASSERT_INTERFACE(ISampler, Sampler);
} // Shift::VK

#endif //SHIFT_VKSAMPLER_HPP
