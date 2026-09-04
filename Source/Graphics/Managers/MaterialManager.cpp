//
// Created by otrush on 9/1/2026.
//

#include "MaterialManager.hpp"

#include "Config/EngineConfig.hpp"
#include "Utility/Assertions.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::Graphics {

    GPU::MaterialData MaterialDataFromDesc(const MaterialDesc& desc, const TextureSlotResolver& resolve) {
        GPU::MaterialData material{
            .baseColorFactor = desc.baseColorFactor,
            .emissiveFactor = desc.emissiveFactor,
            .alphaCutoff = desc.alphaCutoff,
            .baseColorTex = resolve(desc.baseColor),
            .normalTex = resolve(desc.normal),
            .ormTex = resolve(desc.orm),
            .emissiveTex = resolve(desc.emissive),
            .metallicFactor = desc.metallicFactor,
            .roughnessFactor = desc.roughnessFactor,
            .flags = desc.alphaTest //! For now
        };

        return material;
    }

    bool MaterialManager::Init(const TextureSlotResolver& resolve) {
        m_materials.clear();
        const uint32_t defaultIndex = RegisterMaterial(MaterialDesc{}, resolve);

        CheckCritical(defaultIndex == DEFAULT_MATERIAL_INDEX,
                      "The default material did not land in slot 0 of the material array!");
        return true;
    }

    uint32_t MaterialManager::RegisterMaterial(const MaterialDesc& desc, const TextureSlotResolver& resolve) {
        if (m_materials.size() >= Conf::MAX_SCENE_MATERIALS) {
            Log(Error, "Material array is full at {} entries; material '{}' falls back to the default",
                Conf::MAX_SCENE_MATERIALS, desc.name);
            return DEFAULT_MATERIAL_INDEX;
        }

        const uint32_t index = static_cast<uint32_t>(m_materials.size());
        m_materials.push_back(MaterialDataFromDesc(desc, resolve));
        return index;
    }

    std::vector<uint32_t> MaterialManager::RegisterModelMaterials(const std::vector<MaterialDesc>& materials,
                                                                   const TextureSlotResolver& resolve) {
        std::vector<uint32_t> remap;
        remap.reserve(materials.size());
        for (const MaterialDesc& desc : materials) {
            remap.push_back(RegisterMaterial(desc, resolve));
        }
        return remap;
    }

    uint32_t MaterialManager::Resolve(std::span<const uint32_t> remap, uint32_t modelLocalIndex) {
        if (modelLocalIndex == MODEL_INDEX_NONE || modelLocalIndex >= remap.size()) {
            return DEFAULT_MATERIAL_INDEX;
        }
        return remap[modelLocalIndex];
    }
}
