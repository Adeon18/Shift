//
// Created by otrush on 8/30/2026.
//

#include "RenderScene.hpp"

#include <algorithm>

#include "Scene/ShiftScene.hpp"

namespace Shift::Graphics {

    void RenderScene::Extract(const ShiftScene& scene, const MeshManager& meshes, PipelineHandle forwardPipeline) {
        m_objects.clear();
        m_drawItems.clear();
        m_stats = {};

        const auto& registry = scene.GetRegistry();
        registry.view<MeshProperty>().each([this, &scene, &meshes, &forwardPipeline](const auto entity, const auto& meshProp) {
            const Mesh* mesh = meshes.Get(meshProp.mesh);
            const glm::mat4 transform = scene.GetWorld(entity);
            if (!mesh) { return; }
            GPU::ObjectData data{};
            uint32_t objectIndex = static_cast<uint32_t>(m_objects.size());
            data.model = transform;
            const glm::vec4 localSphere = mesh->bounds.sphere;
            const float radiusScale = std::max({glm::length(glm::vec3(transform[0])),
                                                glm::length(glm::vec3(transform[1])),
                                                glm::length(glm::vec3(transform[2]))});
            data.boundsSphere = glm::vec4(
                glm::vec3(transform * glm::vec4(glm::vec3(localSphere), 1.0f)),
                localSphere.w * radiusScale);
            m_objects.push_back(data);

            for (uint32_t i = 0; i < mesh->submeshes.size(); ++i) {
                const MeshSubmesh& sm = mesh->submeshes[i];
                //! This could have used the material index commented out below, but since we allow material replacement
                //! the indices will differ after the first replacement
                // const uint32_t materialIndex = sm.materialIndex;
                DrawItem dW{
                    .passMask = EPassBit::Forward | EPassBit::DepthOnly,
                    .pipeline = forwardPipeline,
                    .firstIndex = mesh->indexRange.first + sm.firstIndex,
                    .indexCount = sm.indexCount,
                    .objectIndex = objectIndex,
                    .instanceCount = 1,
                    .materialIndex = meshProp.materials[i],
                    .meshVertexBase = mesh->vertexRange.first,
                    .sortKey = MakeDrawSortKey(forwardPipeline.slotIdx, meshProp.mesh.slotIdx, i, meshProp.materials[i])
                };
                m_drawItems.push_back(dW);
            }
        });

        std::sort(m_drawItems.begin(), m_drawItems.end(),
                  [](const DrawItem& lhs, const DrawItem& rhs) { return lhs.sortKey < rhs.sortKey; });

        m_stats.objects = static_cast<uint32_t>(m_objects.size());
        m_stats.culled = 0;
        m_stats.drawCalls = static_cast<uint32_t>(m_drawItems.size());
        m_stats.instances = static_cast<uint32_t>(m_drawItems.size()); // FOR NOW
    }
}
