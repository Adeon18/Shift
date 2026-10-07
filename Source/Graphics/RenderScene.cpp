//
// Created by otrush on 8/30/2026.
//

#include "RenderScene.hpp"

#include <algorithm>

#include "Scene/ShiftScene.hpp"

namespace Shift::Graphics {

    namespace {
        //! One submesh data needed to build draw items later
        struct SubmeshRecord {
            uint64_t sortKey = 0;
            uint32_t objectIndex = 0;
            uint32_t submeshIndex = 0;
            uint32_t materialIndex = 0;
            EMaterialRasterFlags rasterFlags = EMaterialRasterFlags::None;
            const Mesh* mesh = nullptr;
        };

        //! Get world sphere from local space
        [[nodiscard]] glm::vec4 WorldSphere(const glm::mat4& transform, const glm::vec4& localSphere) {
            const float radiusScale = std::max({glm::length(glm::vec3(transform[0])),
                                                glm::length(glm::vec3(transform[1])),
                                                glm::length(glm::vec3(transform[2]))});
            return glm::vec4(glm::vec3(transform * glm::vec4(glm::vec3(localSphere), 1.0f)), localSphere.w * radiusScale);
        }
    }

    void RenderScene::Extract(const ShiftScene& scene, const MeshManager& meshes, const MaterialManager& materials, const ExtractInputs& inputs) {
        m_objects.clear();
        m_drawItems.clear();
        m_submeshInstances.clear();
        m_stats = {};

        std::vector<SubmeshRecord> records;

        const auto& registry = scene.GetRegistry();
        registry.view<MeshProperty>().each([this, &scene, &meshes, &materials, &inputs, &records](const auto entity, const auto& meshProp) {
            const Mesh* mesh = meshes.Get(meshProp.mesh);
            const glm::mat4 transform = scene.GetWorld(entity);
            if (!mesh) { return; }
            //! W esubmit object data regardless, culling is controlled by isntances
            GPU::ObjectData data{};
            uint32_t objectIndex = static_cast<uint32_t>(m_objects.size());
            data.model = transform;
            data.boundsSphere = WorldSphere(transform, mesh->bounds.sphere);
            m_objects.push_back(data);

            for (uint32_t i = 0; i < mesh->submeshes.size(); ++i) {
                //! Per sumhesh
                if (!IsVisible(inputs.frustum, WorldSphere(transform, mesh->submeshes[i].bounds.sphere))) {
                    ++m_stats.culled;
                    continue;
                }
                //! This could have used the material index commented out below, but since we allow material replacement
                //! the indices will differ after the first replacement
                // const uint32_t materialIndex = sm.materialIndex;
                const uint32_t materialIndex = meshProp.materials[i];
                //! The material decides the raster state
                const EMaterialRasterFlags rasterFlags = materials.GetRasterFlags(materialIndex);
                records.push_back({
                    .sortKey = MakeDrawSortKey(static_cast<uint32_t>(rasterFlags), meshProp.mesh.slotIdx, i, materialIndex),
                    .objectIndex = objectIndex,
                    .submeshIndex = i,
                    .materialIndex = materialIndex,
                    .rasterFlags = rasterFlags,
                    .mesh = mesh
                });
            }
        });

        //! Sort by keyss
        std::sort(records.begin(), records.end(), [](const SubmeshRecord& lhs, const SubmeshRecord& rhs) { return lhs.sortKey < rhs.sortKey; });

        m_submeshInstances.reserve(records.size());
        //! Ranged iterations, so for each record, we check how many same key recordds are there and emit one draw item for all with respective index count and shit
        for (size_t runStart = 0, runEnd = 0; runStart < records.size(); runStart = runEnd) {
            const SubmeshRecord& first = records[runStart];
            while (runEnd < records.size() && records[runEnd].sortKey == first.sortKey) {
                m_submeshInstances.push_back(records[runEnd].objectIndex);
                ++runEnd;
            }

            const MeshSubmesh& sm = first.mesh->submeshes[first.submeshIndex];
            m_drawItems.push_back({
                .passMask = EPassBit::Forward | EPassBit::DepthOnly,
                .rasterFlags = first.rasterFlags,
                .firstIndex = first.mesh->indexRange.first + sm.firstIndex,
                .indexCount = sm.indexCount,
                .firstSubmeshInstance = static_cast<uint32_t>(runStart),
                .instanceCount = static_cast<uint32_t>(runEnd - runStart),
                .materialIndex = first.materialIndex,
                .meshVertexBase = first.mesh->vertexRange.first,
                .sortKey = first.sortKey
            });
        }

        m_stats.objects = static_cast<uint32_t>(m_objects.size());
        m_stats.drawCalls = static_cast<uint32_t>(m_drawItems.size());
        m_stats.instances = static_cast<uint32_t>(m_submeshInstances.size());
    }
}
