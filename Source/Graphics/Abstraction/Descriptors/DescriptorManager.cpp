#include "DescriptorManager.hpp"

namespace shift::gfx {
    DescriptorManager::DescriptorManager(const shift::gfx::Device &device): m_device{device} {
        m_framePool = std::make_unique<DescriptorPool>(m_device);
        m_viewPool = std::make_unique<DescriptorPool>(m_device);
        m_matPool = std::make_unique<DescriptorPool>(m_device);
        m_objPool = std::make_unique<DescriptorPool>(m_device);
    }

    bool DescriptorManager::AllocatePools() {
        m_framePool->AddUBOSize(5);
        m_framePool->AddSamplerSize(gutil::SHIFT_MAX_FRAMES_IN_FLIGHT); // Debug
        m_framePool->SetMaxSets(gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        if (!m_framePool->Build()) return false;

        m_viewPool->AddUBOSize(5);
        m_viewPool->SetMaxSets(5 * gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        if (!m_viewPool->Build()) return false;

        m_matPool->AddUBOSize(3);
        m_matPool->AddSamplerSize(10);
        m_matPool->SetMaxSets(10 * gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        if (!m_matPool->Build()) return false;

        m_objPool->AddUBOSize(3);
        m_objPool->SetMaxSets(10 * gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        if (!m_objPool->Build()) return false;

        return true;
    }

    DescriptorSet &DescriptorManager::CreatePerFrameSet(uint32_t frameIdx) {
        m_perFrameSets[frameIdx] = std::make_unique<DescriptorSet>(m_device);
        return *m_perFrameSets[frameIdx];
    }

    DescriptorSet &DescriptorManager::CreatePerViewSet(uint32_t viewID, uint32_t frameIdx) {
        m_perViewSets[viewID][frameIdx] = std::make_unique<DescriptorSet>(m_device);
        return *m_perViewSets[viewID][frameIdx];
    }

    DescriptorSet &DescriptorManager::CreatePerMaterialSet(PassType type, uint32_t frameIdx) {
        m_perMaterialSets[type][frameIdx] = std::make_unique<DescriptorSet>(m_device);
        return *m_perMaterialSets[type][frameIdx];
    }

    DescriptorSet &DescriptorManager::CreatePerObjectSet(ObjectType type, uint32_t frameIdx) {
        m_perObjectSets[type][frameIdx] = std::make_unique<DescriptorSet>(m_device);
        return *m_perObjectSets[type][frameIdx];
    }

    bool DescriptorManager::AllocateAll() {
        for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            if (m_perFrameSets[i]) { [[unlikely]] if (!m_perFrameSets[i]->Allocate(m_framePool->Get())) { return false; } }

            for (auto& [k,v]: m_perViewSets) {
                if (v[i]) { [[unlikely]] if (!v[i]->Allocate(m_viewPool->Get())) { return false; } }
            }

            for (auto& [k,v]: m_perMaterialSets) {
                if (v[i]) { [[unlikely]] if (!v[i]->Allocate(m_matPool->Get())) { return false; } }
            }

            for (auto& [k,v]: m_perObjectSets) {
                if (v[i]) { [[unlikely]] if (!v[i]->Allocate(m_objPool->Get())) { return false; } }
            }
        }

        return true;
    }
} // shift::gfx