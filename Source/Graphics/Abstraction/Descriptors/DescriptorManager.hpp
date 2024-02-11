#ifndef SHIFT_DESCRIPTORMANAGER_HPP
#define SHIFT_DESCRIPTORMANAGER_HPP

#include <unordered_map>
#include <array>
#include <optional>
#include <memory>

#include "DescriptorManagement.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"

#include "Utility/Vulkan/UtilVulkan.hpp"

namespace sft::gfx {
    enum class MaterialType {
        EMISSION_ONLY,
        TEXTURED
    };

    enum class ObjectType {
        DEFAULT
    };

    class DescriptorManager {
    public:
        DescriptorManager(const Device& device);

        [[nodiscard]] bool AllocatePools();

        //! Creates a set entry perframe set that you can externally configure
        [[nodiscard]] DescriptorSet& CreatePerFrameSet();
        //! Creates a set entry per view set that you can externally configure
        [[nodiscard]] DescriptorSet& CreatePerViewSet(uint32_t viewID);
        //! Creates a set entry per material set that you can externally configure
        [[nodiscard]] DescriptorSet& CreatePerMaterialSet(MaterialType type);
        //! Creates a set entry per object set that you can externally configure
        [[nodiscard]] DescriptorSet& CreatePerObjectSet(ObjectType type);

        //! Get reference to the respective frame index of the per frame set
        [[nodiscard]] DescriptorSet& GetPerFrameSet(uint32_t frameIdx) { return *m_perFrameSets[frameIdx]; }
        //! Get reference to the respective frame index of the per view sets
        [[nodiscard]] DescriptorSet& GetPerViewSet(uint32_t viewID, uint32_t frameIdx) { return *m_perViewSets[viewID][frameIdx]; }
        //! Get reference to the respective frame index of the per material sets
        [[nodiscard]] DescriptorSet& GetPerMaterialSet(MaterialType type, uint32_t frameIdx) { return *m_perMaterialSets[type][frameIdx]; }
        //! Get reference to the respective frame index of the per object sets
        [[nodiscard]] DescriptorSet& GetPerObjectSet(ObjectType type, uint32_t frameIdx) { return *m_perObjectSets[type][frameIdx]; }

        [[nodiscard]] bool Build();

        ~DescriptorManager() = default;

        DescriptorManager() = delete;
        DescriptorManager(const DescriptorManager&) = delete;
        DescriptorManager& operator=(const DescriptorManager&) = delete;
    private:
        const Device& m_device;

        //! The storage of stats
        std::vector<std::unique_ptr<DescriptorSet>> m_perFrameSets;
        std::unordered_map<uint32_t, std::vector<std::unique_ptr<DescriptorSet>>> m_perViewSets;
        std::unordered_map<MaterialType, std::vector<std::unique_ptr<DescriptorSet>>> m_perMaterialSets;
        std::unordered_map<ObjectType, std::vector<std::unique_ptr<DescriptorSet>>> m_perObjectSets;


        //! I have a pool for different set types
        std::unique_ptr<DescriptorPool> m_framePool;
        std::unique_ptr<DescriptorPool> m_viewPool;
        std::unique_ptr<DescriptorPool> m_matPool;
        std::unique_ptr<DescriptorPool> m_objPool;
    };
} // sft::gfx

#endif //SHIFT_DESCRIPTORMANAGER_HPP
