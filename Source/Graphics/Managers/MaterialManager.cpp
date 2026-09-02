//
// Created by otrush on 9/1/2026.
//

#include "MaterialManager.hpp"

#include "Config/EngineConfig.hpp"
#include "Utility/Assertions.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::Graphics {

    GPU::MaterialData MaterialDataFromDesc(const MaterialDesc& desc) {
        GPU::MaterialData material{
            .baseColorFactor = desc.baseColorFactor,
            .emissiveFactor = desc.emissiveFactor,
            .alphaCutoff = desc.alphaCutoff,
            .baseColorTex = 0,
            .normalTex = 0,
            .ormTex = 0,
            .emissiveTex = 0,
            .metallicFactor = desc.metallicFactor,
            .roughnessFactor = desc.roughnessFactor,
            .flags = desc.alphaTest //! For now
        };

        return material;
    }

    bool MaterialManager::Init() {
        m_materials.clear();
        const uint32_t defaultIndex = RegisterMaterial(MaterialDesc{});

        CheckCritical(defaultIndex == DEFAULT_MATERIAL_INDEX,
                      "The default material did not land in slot 0 of the material array!");
        return true;
    }

    uint32_t MaterialManager::RegisterMaterial(const MaterialDesc& desc) {
        if (m_materials.size() >= Conf::MAX_SCENE_MATERIALS) {
            Log(Error, "Material array is full at {} entries; material '{}' falls back to the default",
                Conf::MAX_SCENE_MATERIALS, desc.name);
            return DEFAULT_MATERIAL_INDEX;
        }

        const uint32_t index = static_cast<uint32_t>(m_materials.size());
        m_materials.push_back(MaterialDataFromDesc(desc));
        return index;
    }

    std::vector<uint32_t> MaterialManager::RegisterModelMaterials(const std::vector<MaterialDesc>& materials) {
        std::vector<uint32_t> remap;
        remap.reserve(materials.size());
        for (const MaterialDesc& desc : materials) {
            remap.push_back(RegisterMaterial(desc));
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
