#ifndef SHIFT_VKSAMPLER_HPP
#define SHIFT_VKSAMPLER_HPP

#include "VKDevice.hpp"

#include "Graphics/RHI/Sampler.hpp"

namespace Shift::VK {
    class Sampler {
        friend VK::ResourceSet;
    public:
        Sampler()=default;

        //! Initialize a sampler with the RHI desc
        //! \param device The VkDevice
        //! \param desc RHI desc
        //! \return true if successful, false otherwise
        [[nodiscard]] void Init(const Device* device, const SamplerDescriptor& desc);

        [[nodiscard]] bool IsValid() const { return valid; }

        void Destroy();
        ~Sampler() = default;
    private:
        //! API SPECIFIC, DO NOT USE UNLESS NESSESARY IN RHI SPECIFIC CODE
        //! \return VkPipeline
        [[nodiscard]] VkSampler VK_Get() const { return m_sampler; }
    private:
        const Device* m_device = nullptr;

        VkSampler m_sampler;
        bool valid = false;
    };

    ASSERT_INTERFACE(ISampler, Sampler);
} // Shift::VK

#endif //SHIFT_VKSAMPLER_HPP
