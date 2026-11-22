#ifndef SHIFT_DX12RESOURCESET_HPP
#define SHIFT_DX12RESOURCESET_HPP

#include <cstdint>

#include "DX12Device.hpp"
#include "../Common/ResourceSet.hpp"

namespace Shift::DX12 {
    class ResourceSet {
    public:
        [[nodiscard]] bool IsValid() const;

        void UpdateUBO(uint32_t bind, const Buffer& InputBuffer);
        void UpdateUBO(uint32_t bind, const Buffer& InputBuffer, uint32_t size, uint32_t offset);
        void UpdateTexture(uint32_t bind, uint32_t arrElement, const Texture& InputTexture);
        void UpdateSampler(uint32_t bind, uint32_t arrElement, const Sampler& InputSampler);
        void Apply();

        ~ResourceSet();
    };

    ASSERT_INTERFACE(IResourceSet, ResourceSet);
} // Shift::DX12

#endif //SHIFT_DX12RESOURCESET_HPP
