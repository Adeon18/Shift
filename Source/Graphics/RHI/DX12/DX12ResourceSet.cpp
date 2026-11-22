#include "DX12ResourceSet.hpp"

#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    bool ResourceSet::IsValid() const { SRHI_NOT_IMPLEMENTED(); }

    void ResourceSet::UpdateUBO(uint32_t bind, const Buffer& InputBuffer) { SRHI_NOT_IMPLEMENTED(); }
    void ResourceSet::UpdateUBO(uint32_t bind, const Buffer& InputBuffer, uint32_t size, uint32_t offset) { SRHI_NOT_IMPLEMENTED(); }
    void ResourceSet::UpdateTexture(uint32_t bind, uint32_t arrElement, const Texture& InputTexture) { SRHI_NOT_IMPLEMENTED(); }
    void ResourceSet::UpdateSampler(uint32_t bind, uint32_t arrElement, const Sampler& InputSampler) { SRHI_NOT_IMPLEMENTED(); }
    void ResourceSet::Apply() { SRHI_NOT_IMPLEMENTED(); }

    ResourceSet::~ResourceSet() = default;
} // Shift::DX12
