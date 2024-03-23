#include "DescriptorManager.hpp"

namespace shift::gfx {
    DescriptorManager::DescriptorManager(const shift::gfx::Device &device): m_device{device} {
        m_framePool = std::make_unique<DescriptorPool>(m_device);
        m_viewPool = std::make_unique<DescriptorPool>(m_device);
        m_matPool = std::make_unique<DescriptorPool>(m_device);
        m_ImGuiPool = std::make_unique<DescriptorPool>(m_device);

        CreateImGuiLayout(
                {
                        {DescriptorType::SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT}
                }
        );
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
        m_matPool->SetMaxSets(2048 * gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        if (!m_matPool->Build()) return false;

        m_ImGuiPool->AddUBOSize(10);
        m_ImGuiPool->AddSamplerSize(10);
        m_ImGuiPool->SetMaxSets(100 * gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        if (!m_ImGuiPool->Build(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)) return false;


        return true;
    }

    // TODO: this can definitely be moved into a fancy template but my ass is on fire
    SGUID DescriptorManager::AllocatePerFrameSet() {
        for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_perFrameSets[i] = std::make_unique<DescriptorSet>(m_device);
            m_perFrameSets[i]->Allocate(m_framePool->Get(), m_perFrameLayout->Get());
        }
        return 0;
    }

    SGUID DescriptorManager::AllocatePerViewSet(ViewSetLayoutType type) {
        SGUID id = GUIDGenerator::GetInstance().Guid();
        m_perViewSets[id].type = type;
        for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_perViewSets[id].setsInFlight[i] = std::make_unique<DescriptorSet>(m_device);
            m_perViewSets[id].setsInFlight[i]->Allocate(m_viewPool->Get(), m_perViewTypeLayouts[type]->Get());
        }
        return id;
    }

    SGUID DescriptorManager::AllocatePerMaterialSet(MaterialSetLayoutType type) {
        SGUID id = GUIDGenerator::GetInstance().Guid();
        m_perMaterialSets[id].type = type;
        for (uint32_t i = 0; i < gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_perMaterialSets[id].setsInFlight[i] = std::make_unique<DescriptorSet>(m_device);
            m_perMaterialSets[id].setsInFlight[i]->Allocate(m_matPool->Get(),
                                                               m_perMaterialTypeLayouts[type]->Get());
        }
        return id;
    }

    SGUID DescriptorManager::AllocateImGuiSet(ImGuiSetLayoutType type) {
        SGUID id = GUIDGenerator::GetInstance().Guid();
        m_imguiTextureSets[id] = std::make_unique<DescriptorSet>(m_device);
        m_imguiTextureSets[id]->Allocate(m_ImGuiPool->Get(), m_imguiLayout->Get());
        return id;
    }

    bool DescriptorManager::CreatePerFrameLayout(const std::vector<DescriptorLayoutEntry> &entries) {
        m_perFrameLayout = std::make_unique<DescriptorLayout>(m_device);
        for (auto& entry: entries) {
            FillDescriptorLayoutEntryData(*m_perFrameLayout, entry);
        }
        return m_perFrameLayout->Build();
    }

    bool
    DescriptorManager::CreatePerViewLayout(ViewSetLayoutType type, const std::vector<DescriptorLayoutEntry> &entries) {
        m_perViewTypeLayouts[type] = std::make_unique<DescriptorLayout>(m_device);
        for (auto& entry: entries) {
            FillDescriptorLayoutEntryData(*m_perViewTypeLayouts[type], entry);
        }
        return m_perViewTypeLayouts[type]->Build();
    }

    bool DescriptorManager::CreatePerMaterialLayout(MaterialSetLayoutType type,
                                              const std::vector<DescriptorLayoutEntry> &entries) {
        m_perMaterialTypeLayouts[type] = std::make_unique<DescriptorLayout>(m_device);
        for (auto& entry: entries) {
            FillDescriptorLayoutEntryData(*m_perMaterialTypeLayouts[type], entry);
        }
        return m_perMaterialTypeLayouts[type]->Build();
    }

    void
    DescriptorManager::FillDescriptorLayoutEntryData(DescriptorLayout &layout, const DescriptorLayoutEntry &entry) {
        switch (entry.type) {
            case DescriptorType::UBO:
                layout.AddUBOBinding(entry.bind, entry.stages);
                break;
            case DescriptorType::SAMPLER:
                layout.AddSamplerBinding(entry.bind, entry.stages);
                break;
        }
    }

    bool DescriptorManager::CreateImGuiLayout(const std::vector<DescriptorLayoutEntry> &entries) {
        m_imguiLayout = std::make_unique<DescriptorLayout>(m_device);
        for (auto& entry: entries) {
            FillDescriptorLayoutEntryData(*m_imguiLayout, entry);
        }
        return m_imguiLayout->Build();
    }
} // shift::gfx