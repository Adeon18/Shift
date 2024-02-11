#include "DescriptorManager.hpp"

namespace sft::gfx {
    DescriptorManager::DescriptorManager(const sft::gfx::Device &device): m_device{device} {
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
    }

    DescriptorSet &DescriptorManager::CreatePerFrameSet() {
        m_perFrameSets.resize(gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_perFrameSets[0] = std::make_unique<DescriptorSet>(m_device);
        return *m_perFrameSets[0];
    }

    DescriptorSet &DescriptorManager::CreatePerViewSet(uint32_t viewID) {
        m_perViewSets[viewID].resize(gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_perViewSets[viewID][0] = std::make_unique<DescriptorSet>(m_device);
        return *m_perViewSets[viewID][0];
    }

    DescriptorSet &DescriptorManager::CreatePerMaterialSet(MaterialType type) {
        m_perMaterialSets[type].resize(gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_perMaterialSets[type][0] = std::make_unique<DescriptorSet>(m_device);
        return *m_perMaterialSets[type][0];
    }

    DescriptorSet &DescriptorManager::CreatePerObjectSet(ObjectType type) {
        m_perObjectSets[type].resize(gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_perObjectSets[type][0] = std::make_unique<DescriptorSet>(m_device);
        return *m_perObjectSets[type][0];
    }

    bool DescriptorManager::Build() {
        [[unlikely]] if (!m_perFrameSets[0]->Build(m_framePool->Get())) {return false;}

        for (auto& [k,v]: m_perViewSets) {
            [[unlikely]] if (!v[0]->Build(m_viewPool->Get())) {return false;}
        }

        for (auto& [k,v]: m_perMaterialSets) {
            [[unlikely]] if (!v[0]->Build(m_matPool->Get())) {return false;}
        }

        for (auto& [k,v]: m_perObjectSets) {
            [[unlikely]] if (!v[0]->Build(m_objPool->Get())) {return false;}
        }

        for (uint32_t i = 1; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_perFrameSets[i] = std::make_unique<DescriptorSet>(m_device);
            m_perFrameSets[0]->CopyForFlight(*m_perFrameSets[i]);
            [[unlikely]] if (!m_perFrameSets[i]->Build(m_framePool->Get())) {return false;}

            for (auto& [k,v]: m_perViewSets) {
                v[i]= std::make_unique<DescriptorSet>(m_device);
                v[0]->CopyForFlight(*v[i]);
                [[unlikely]] if (!v[i]->Build(m_viewPool->Get())) {return false;}
            }

            for (auto& [k,v]: m_perMaterialSets) {
                v[i] = std::make_unique<DescriptorSet>(m_device);
                v[0]->CopyForFlight(*v[i]);
                [[unlikely]]  if (!v[i]->Build(m_matPool->Get())) {return false;}
            }

            for (auto& [k,v]: m_perObjectSets) {
                v[i] = std::make_unique<DescriptorSet>(m_device);
                v[0]->CopyForFlight(*v[i]);
                [[unlikely]] if (!v[i]->Build(m_objPool->Get())) {return false;}
            }
        }

        return true;
    }
} // sft::gfx