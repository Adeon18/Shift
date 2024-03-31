#ifndef SHIFT_DESCRIPTORMANAGER_HPP
#define SHIFT_DESCRIPTORMANAGER_HPP

#include <unordered_map>
#include <array>
#include <optional>
#include <memory>

#include "DescriptorManagement.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"

#include "Utility/Vulkan/UtilVulkan.hpp"

#include "Utility/GUIDGenerator/GUIDGenerator.hpp"


namespace shift::gfx {
    enum class MaterialSetLayoutType {
        EMISSION_ONLY,
        TEXTURED,
        PBR,
        //! TODO: NO, this is bad design
        POST_PROCESS
    };

    enum class ViewSetLayoutType {
        DEFAULT_CAMERA
    };

    enum class ImGuiSetLayoutType {
        TEXTURE,
        RENDER_TARGET
    };

    enum class ObjectType {
        DEFAULT
    };

    enum class DescriptorType {
        UBO,
        SAMPLER
    };

    struct DescriptorLayoutEntry {
        DescriptorType type;
        uint32_t bind;
        VkShaderStageFlags stages;
    };

    class DescriptorManager {
    public:
        DescriptorManager(const Device& device);

        [[nodiscard]] bool AllocatePools();

        bool CreatePerFrameLayout(const std::vector<DescriptorLayoutEntry>& entries);
        bool CreatePerViewLayout(ViewSetLayoutType type, const std::vector<DescriptorLayoutEntry>& entries);
        bool CreatePerMaterialLayout(MaterialSetLayoutType type, const std::vector<DescriptorLayoutEntry>& entries);
        bool CreateImGuiLayout(const std::vector<DescriptorLayoutEntry>& entries);

        [[nodiscard]] const DescriptorPool& GetImGuiPool() const { return *m_ImGuiPool; }

        [[nodiscard]] DescriptorLayout& GetPerFrameLayout() { return *m_perFrameLayout; }
        [[nodiscard]] DescriptorLayout& GetPerViewLayout(ViewSetLayoutType type) { return *m_perViewTypeLayouts[type]; }
        [[nodiscard]] DescriptorLayout& GetPerMaterialLayout(MaterialSetLayoutType type) { return *m_perMaterialTypeLayouts[type]; }

        [[nodiscard]] DescriptorLayout& GetImGuiLayout() { return *m_imguiLayout; }

        //! Creates a set entry perframe set that you can externally configure
        [[nodiscard]] SGUID AllocatePerFrameSet();
        //! Creates a set entry per view set that you can externally configure, returns it's GUID
        [[nodiscard]] SGUID AllocatePerViewSet(ViewSetLayoutType type);
        //! Creates a set entry per material set that you can externally configure, returns it's GUID
        [[nodiscard]] SGUID AllocatePerMaterialSet(MaterialSetLayoutType type);

        //! Allocate a descriptor set for ImGui images
        [[nodiscard]] SGUID AllocateImGuiSet(ImGuiSetLayoutType type);

        //! Get reference to the respective frame index of the per frame set
        [[nodiscard]] DescriptorSet& GetPerFrameSet(uint32_t frameIdx) { return *m_perFrameSets[frameIdx]; }
        //! Get reference to the respective frame index of the per view sets
        [[nodiscard]] DescriptorSet& GetPerViewSet(SGUID id, uint32_t frameIdx) { return *m_perViewSets[id].setsInFlight[frameIdx]; }
        //! Get reference to the respective frame index of the per material sets
        [[nodiscard]] DescriptorSet& GetPerMaterialSet(SGUID id, uint32_t frameIdx) { return *m_perMaterialSets[id].setsInFlight[frameIdx]; }

        // TODO: For now
        [[nodiscard]] DescriptorSet& GetImGuiSet(ImGuiSetLayoutType type, SGUID id, uint32_t frameIdx=0) { return *m_imguiTextureSets[id]; }
        ~DescriptorManager() = default;

        DescriptorManager() = delete;
        DescriptorManager(const DescriptorManager&) = delete;
        DescriptorManager& operator=(const DescriptorManager&) = delete;
    private:
        //! Fill one entry for descriptor layout depending on type
        static void FillDescriptorLayoutEntryData(DescriptorLayout& layout, const DescriptorLayoutEntry& entry);

        using DescriptorSetsInFlight = std::array<std::unique_ptr<DescriptorSet>, gutil::SHIFT_MAX_FRAMES_IN_FLIGHT>;
        template<typename T>
        struct DescriptorSetData {
            T type;
            std::array<std::unique_ptr<DescriptorSet>, gutil::SHIFT_MAX_FRAMES_IN_FLIGHT> setsInFlight;
        };

        //! Layouts
        std::unique_ptr<DescriptorLayout> m_perFrameLayout;
        std::unordered_map<ViewSetLayoutType, std::unique_ptr<DescriptorLayout>> m_perViewTypeLayouts;
        std::unordered_map<MaterialSetLayoutType, std::unique_ptr<DescriptorLayout>> m_perMaterialTypeLayouts;

        std::unique_ptr<DescriptorLayout> m_imguiLayout;

        const Device& m_device;

        //! Sets
        DescriptorSetsInFlight m_perFrameSets;
        std::unordered_map<SGUID, DescriptorSetData<ViewSetLayoutType>> m_perViewSets;
        std::unordered_map<SGUID, DescriptorSetData<MaterialSetLayoutType>> m_perMaterialSets;

        std::unordered_map<SGUID, std::unique_ptr<DescriptorSet>> m_imguiTextureSets;


        //! I have a pool for different set types
        std::unique_ptr<DescriptorPool> m_framePool;
        std::unique_ptr<DescriptorPool> m_viewPool;
        std::unique_ptr<DescriptorPool> m_matPool;

        std::unique_ptr<DescriptorPool> m_ImGuiPool;
    };
} // shift::gfx

#endif //SHIFT_DESCRIPTORMANAGER_HPP
