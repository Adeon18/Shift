#ifndef SHIFT_DX12SAMPLER_HPP
#define SHIFT_DX12SAMPLER_HPP

#include "DX12Device.hpp"
#include "../Common/Sampler.hpp"

namespace Shift::DX12 {
    class Sampler {
    public:
        Sampler(const Device* device, const SamplerDescriptor& desc);

        [[nodiscard]] bool IsValid() const;

        ~Sampler();
    };

    ASSERT_INTERFACE(ISampler, Sampler);
} // Shift::DX12

#endif //SHIFT_DX12SAMPLER_HPP
