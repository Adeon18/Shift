//
// Created by otrush on 8/14/2026.
//
#include "SamplerManager.hpp"

#include "Config/EngineConfig.hpp"
#include "Graphics/Shared/GPUShared.h"

namespace Shift::Graphics {
    namespace {
        bool SameSamplerState(const SamplerDescriptor& a, const SamplerDescriptor& b) {
            const bool sameFiltering = a.mipFilter == b.mipFilter &&
                                       a.minFilter == b.minFilter &&
                                       a.magFilter == b.magFilter;
            const bool sameAddressing = a.addressModeU == b.addressModeU &&
                                        a.addressModeV == b.addressModeV &&
                                        a.addressModeW == b.addressModeW &&
                                        a.unnormalizedCoordinates == b.unnormalizedCoordinates;
            const bool sameBorder = a.borderColor[0] == b.borderColor[0] &&
                                    a.borderColor[1] == b.borderColor[1] &&
                                    a.borderColor[2] == b.borderColor[2] &&
                                    a.borderColor[3] == b.borderColor[3];
            const bool sameCompare = a.compareEnable == b.compareEnable &&
                                     a.compareFunction == b.compareFunction;
            const bool sameLod = a.mipLodBias == b.mipLodBias &&
                                 a.minLod == b.minLod &&
                                 a.maxLod == b.maxLod &&
                                 a.maxAnisotropy == b.maxAnisotropy;

            return sameFiltering && sameAddressing && sameBorder && sameCompare && sameLod;
        }

        SamplerDescriptor LinearRepeatDesc() {
            return SamplerDescriptor{
                .mipFilter = EMipMapMode::Linear,
                .minFilter = EFilterMode::Linear,
                .magFilter = EFilterMode::Linear,
                .name = "LinearRepeat"
            };
        }

        SamplerDescriptor LinearClampDesc() {
            return SamplerDescriptor{
                .mipFilter = EMipMapMode::Linear,
                .minFilter = EFilterMode::Linear,
                .magFilter = EFilterMode::Linear,
                .addressModeU = ESamplerAddressMode::ClampEdge,
                .addressModeV = ESamplerAddressMode::ClampEdge,
                .addressModeW = ESamplerAddressMode::ClampEdge,
                .name = "LinearClamp"
            };
        }

        SamplerDescriptor NearestRepeatDesc() {
            return SamplerDescriptor{
                .mipFilter = EMipMapMode::Nearest,
                .minFilter = EFilterMode::Nearest,
                .magFilter = EFilterMode::Nearest,
                .name = "NearestRepeat"
            };
        }

        SamplerDescriptor AnisoRepeatDesc() {
            return SamplerDescriptor{
                .mipFilter = EMipMapMode::Linear,
                .minFilter = EFilterMode::Linear,
                .magFilter = EFilterMode::Linear,
                //! Clamped to the device limit inside the backend, so asking for 16 on hardware
                //! that caps lower is not an error
                .maxAnisotropy = 16.0f,
                .name = "AnisoRepeat"
            };
        }
    }

    bool SamplerManager::Init(RenderBackendInterface* backend, GlobalResourceSet& globalSet) {
        m_backend = backend;
        m_globalSet = &globalSet;

        const uint32_t linearRepeat  = GetOrCreate(LinearRepeatDesc());
        const uint32_t linearClamp   = GetOrCreate(LinearClampDesc());
        const uint32_t nearestRepeat = GetOrCreate(NearestRepeatDesc());
        const uint32_t anisoRepeat   = GetOrCreate(AnisoRepeatDesc());

        CheckCritical(linearRepeat == GPU::SAMPLER_LINEAR_REPEAT, "Sampler slot mismatch: SAMPLER_LINEAR_REPEAT");
        CheckCritical(linearClamp == GPU::SAMPLER_LINEAR_CLAMP, "Sampler slot mismatch: SAMPLER_LINEAR_CLAMP");
        CheckCritical(nearestRepeat == GPU::SAMPLER_NEAREST_REPEAT, "Sampler slot mismatch: SAMPLER_NEAREST_REPEAT");
        CheckCritical(anisoRepeat == GPU::SAMPLER_ANISO_REPEAT, "Sampler slot mismatch: SAMPLER_ANISO_REPEAT");
        CheckCritical(GetCount() == 4u, "Not every named sampler was created");

        Log(Info, "SamplerManager: {} named sampler(s) registered in the global array", GetCount());

        return true;
    }

    void SamplerManager::Destroy() {
        for (Entry& entry : m_samplers) {
            delete entry.sampler;
        }
        m_samplers.clear();

        m_globalSet = nullptr;
        m_backend = nullptr;
    }

    uint32_t SamplerManager::GetOrCreate(const SamplerDescriptor& desc) {
        for (uint32_t i = 0; i < m_samplers.size(); ++i) {
            if (SameSamplerState(m_samplers[i].desc, desc)) {
                return i;
            }
        }

        if (m_samplers.size() >= Conf::MAX_BINDLESS_SAMPLERS) {
            Log(Error, "The global sampler array is full ({} slots); '{}' resolves to slot 0 instead",
                Conf::MAX_BINDLESS_SAMPLERS, desc.name);
            return 0u;
        }

        Sampler* sampler = m_backend->CreateSampler(desc);
        if (sampler == nullptr || !sampler->IsValid()) {
            Log(Error, "Failed to create sampler '{}'; resolving to slot 0", desc.name);
            delete sampler;
            return 0u;
        }

        const uint32_t index = static_cast<uint32_t>(m_samplers.size());
        m_samplers.push_back(Entry{desc, sampler});

        m_globalSet->WriteSampler(index, *sampler);
        m_globalSet->Apply();

        return index;
    }

    Sampler* SamplerManager::Get(uint32_t index) const {
        if (index >= m_samplers.size()) {
            return nullptr;
        }
        return m_samplers[index].sampler;
    }
}
