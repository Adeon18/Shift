//
// Created by otrush on 9/1/2026.
//

#ifndef SHIFT_MATERIALMANAGER_HPP
#define SHIFT_MATERIALMANAGER_HPP

#include <functional>
#include <span>
#include <vector>

#include "Loaders/ModelLoader/IModelLoader.hpp"

#include "Graphics/Shared/GPUShared.h"

namespace Shift::Graphics {

    inline constexpr uint32_t DEFAULT_MATERIAL_INDEX = 0u;

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

    private:
        std::vector<GPU::MaterialData> m_materials;
    };
}

#endif //SHIFT_MATERIALMANAGER_HPP
