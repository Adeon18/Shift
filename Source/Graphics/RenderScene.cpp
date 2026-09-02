//
// Created by otrush on 8/30/2026.
//

#include "RenderScene.hpp"

#include <algorithm>

#include <glm/gtc/matrix_inverse.hpp>

namespace Shift::Graphics {

    void RenderScene::Extract(std::span<const MeshPlacement> placements,
                              const MeshManager& meshes,
                              PipelineHandle forwardPipeline) {
        m_objects.clear();
        m_drawItems.clear();
        m_stats = {};

        for (const auto& p: placements) {
            const Mesh* mesh = meshes.Get(p.mesh);
            if (!mesh) { continue; }
            GPU::ObjectData data{};
            uint32_t objectIndex = static_cast<uint32_t>(m_objects.size());
            data.model = p.transform;
            data.normalMat = glm::inverseTranspose(p.transform);
            const glm::vec4 localSphere = mesh->bounds.sphere;
            const float radiusScale = std::max({glm::length(glm::vec3(p.transform[0])),
                                                glm::length(glm::vec3(p.transform[1])),
                                                glm::length(glm::vec3(p.transform[2]))});
            data.boundsSphere = glm::vec4(
                glm::vec3(p.transform * glm::vec4(glm::vec3(localSphere), 1.0f)),
                localSphere.w * radiusScale);
            m_objects.push_back(data);

            for (uint32_t i = 0; i < mesh->submeshes.size(); ++i) {
                const MeshSubmesh& sm = mesh->submeshes[i];
                const uint32_t materialIndex = sm.materialIndex;
                DrawItem dW{
                    .passMask = EPassBit::Forward | EPassBit::DepthOnly,
                    .pipeline = forwardPipeline,
                    .firstIndex = mesh->indexRange.first + sm.firstIndex,
                    .indexCount = sm.indexCount,
                    .objectIndex = objectIndex,
                    .instanceCount = 1,
                    .materialIndex = materialIndex,
                    .meshVertexBase = mesh->vertexRange.first,
                    .sortKey = MakeDrawSortKey(forwardPipeline.slotIdx, p.mesh.slotIdx, i, materialIndex)
                };
                m_drawItems.push_back(dW);
            }
        }

        std::sort(m_drawItems.begin(), m_drawItems.end(),
                  [](const DrawItem& lhs, const DrawItem& rhs) { return lhs.sortKey < rhs.sortKey; });

        m_stats.placements = static_cast<uint32_t>(placements.size());
        m_stats.objects = static_cast<uint32_t>(m_objects.size());
        m_stats.culled = 0;
        m_stats.drawCalls = static_cast<uint32_t>(m_drawItems.size());
        m_stats.instances = static_cast<uint32_t>(m_drawItems.size()); // FOR NOW
    }
}
