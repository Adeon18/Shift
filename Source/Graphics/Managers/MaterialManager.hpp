//
// Created by otrush on 9/1/2026.
//

#ifndef SHIFT_MATERIALMANAGER_HPP
#define SHIFT_MATERIALMANAGER_HPP

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Loaders/ModelLoader/IModelLoader.hpp"
#include "Utility/EnumFlags.hpp"

#include "Graphics/Shared/GPUShared.h"

namespace Shift::Graphics {

    inline constexpr uint32_t DEFAULT_MATERIAL_INDEX = 0u;

    //! This can work with any pipeline for geo
    enum class EMaterialRasterFlags : uint32_t {
        None        = 0,
        Masked      = 1u << 0,
        DoubleSided = 1u << 1,
    };

    DEFINE_ENUM_CLASS_BITWISE_OPERATORS(EMaterialRasterFlags)

    inline constexpr EMaterialRasterFlags MATERIAL_RASTER_ALL = EMaterialRasterFlags::Masked | EMaterialRasterFlags::DoubleSided;
    inline constexpr uint32_t MATERIAL_RASTER_COMBINATIONS = static_cast<uint32_t>(MATERIAL_RASTER_ALL) + 1u;

    [[nodiscard]] constexpr EMaterialRasterFlags MakeMaterialRasterFlags(bool alphaTest, bool doubleSided) {
        return (alphaTest   ? EMaterialRasterFlags::Masked      : EMaterialRasterFlags::None)
             | (doubleSided ? EMaterialRasterFlags::DoubleSided : EMaterialRasterFlags::None);
    }

    //! Ressolves texture handle to a bindless array index
    using TextureSlotResolver = std::function<uint32_t(const TextureRef&)>;

    //! CPU to GPU converter for Material Data
    [[nodiscard]] GPU::MaterialData MaterialDataFromDesc(const MaterialDesc& desc,
                                                         const TextureSlotResolver& resolve);

    //! Could be a Generational Pool later but I aint streaming materials rn
    class MaterialManager {
    public:
        //! Register the default material
        [[nodiscard]] bool Init(const TextureSlotResolver& resolve);

        //! Register an asset material and remap it to a global MI
        [[nodiscard]] std::vector<uint32_t> RegisterModelMaterials(const std::vector<MaterialDesc>& materials,
                                                                   const TextureSlotResolver& resolve);

        //! Reserve mat index or ret default mat. at fail
        [[nodiscard]] uint32_t RegisterMaterial(const MaterialDesc& desc, const TextureSlotResolver& resolve);

        //! Local to Global MI ressolve
        [[nodiscard]] static uint32_t Resolve(std::span<const uint32_t> remap, uint32_t modelLocalIndex);

        //! The array the frame ring mirrors, global index order
        [[nodiscard]] const std::vector<GPU::MaterialData>& GetMaterials() const { return m_materials; }

        [[nodiscard]] uint32_t GetCount() const { return static_cast<uint32_t>(m_materials.size()); }

        [[nodiscard]] const GPU::MaterialData* Get(uint32_t index) const {
            return (index < m_materials.size()) ? &m_materials[index] : nullptr;
        }

        //! Material name for the editor, empty if none
        [[nodiscard]] std::string_view GetName(uint32_t index) const {
            return (index < m_names.size()) ? std::string_view{m_names[index]} : std::string_view{};
        }

        [[nodiscard]] EMaterialRasterFlags GetRasterFlags(uint32_t index) const {
            return (index < m_rasterFlags.size()) ? m_rasterFlags[index] : EMaterialRasterFlags::None;
        }

    private:
        std::vector<GPU::MaterialData> m_materials;
        //! Same idx as above
        std::vector<std::string> m_names;
        std::vector<EMaterialRasterFlags> m_rasterFlags;
    };
}

#endif //SHIFT_MATERIALMANAGER_HPP
