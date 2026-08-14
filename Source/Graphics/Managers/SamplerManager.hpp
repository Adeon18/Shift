//
// Created by otrush on 8/14/2026.
//

#ifndef SHIFT_SAMPLERMANAGER_HPP
#define SHIFT_SAMPLERMANAGER_HPP

#include <vector>

#include "Graphics/RHI/RHI.hpp"
#include "GlobalResourceSet.hpp"

namespace Shift::Graphics {

    //! Owns every Sampler the engine creates and hands out its index in the global sampler array
    class SamplerManager {
    public:
        [[nodiscard]] bool Init(RenderBackendInterface* backend, GlobalResourceSet& globalSet);
        void Destroy();

        //! Index of this sampler state in the global array, or create and register it
        [[nodiscard]] uint32_t GetOrCreate(const SamplerDescriptor& desc);

        //! nullptr if the index was never registered
        [[nodiscard]] Sampler* Get(uint32_t index) const;

        [[nodiscard]] uint32_t GetCount() const { return static_cast<uint32_t>(m_samplers.size()); }

    private:
        struct Entry {
            SamplerDescriptor desc;
            Sampler* sampler = nullptr;
        };

        RenderBackendInterface* m_backend = nullptr;
        GlobalResourceSet* m_globalSet = nullptr;
        std::vector<Entry> m_samplers;
    };
}

#endif //SHIFT_SAMPLERMANAGER_HPP
