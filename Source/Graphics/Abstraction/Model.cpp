#include "Model.hpp"

#include <iostream>

namespace Shift::gfx {
    void Model::InitWithMeshData(const Device& device, const CommandBuffer& buffer) {
        uint32_t totalVerticeCount = 0;
        uint32_t totalIndiceCount = 0;
        for (auto& mesh : m_meshes) {
            totalVerticeCount += static_cast<uint32_t>(mesh.vertices.size());
            totalIndiceCount += static_cast<uint32_t>(mesh.triangles.size()) * 3u;
        }

        std::vector<Vertex> vertices(totalVerticeCount);
        std::vector<uint16_t> indices(totalIndiceCount);

        uint32_t totalVertexOffset = 0;
        uint32_t totalIndexOffset = 0;
        for (auto& mesh : m_meshes) {
            m_ranges.push_back(MeshRange{ totalVertexOffset, totalIndexOffset, static_cast<uint32_t>(mesh.vertices.size()), static_cast<uint32_t>(mesh.triangles.size() * 3) });
            for (uint32_t i = 0; i < mesh.vertices.size(); ++i) {
                vertices[totalVertexOffset + i] = mesh.vertices[i];
            }
            for (uint32_t i = 0; i < mesh.triangles.size(); ++i) {
                indices[totalIndexOffset + i * 3] = mesh.triangles[i].indices[0];
                indices[totalIndexOffset + i * 3 + 1] = mesh.triangles[i].indices[1];
                indices[totalIndexOffset + i * 3 + 2] = mesh.triangles[i].indices[2];
            }
            totalVertexOffset += static_cast<uint32_t>(mesh.vertices.size());
            totalIndexOffset += static_cast<uint32_t>(mesh.triangles.size()) * 3u;
        }

        m_vertexBuffer = std::make_unique<VertexBuffer>(device, sizeof(vertices[0]) * vertices.size());
        m_indexBuffer = std::make_unique<IndexBuffer>(device, sizeof(indices[0]) * indices.size());

        // TODO: Move this responsibility to the abstraction

        auto vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        Shift::gfx::StagingBuffer stagingBufferVertex{device, vertexBufferSize};
        stagingBufferVertex.Fill(vertices.data(), vertexBufferSize);
        buffer.CopyBuffer(stagingBufferVertex.Get(), m_vertexBuffer->Get(), vertexBufferSize);

        auto indexBufferSize = sizeof(indices[0]) * indices.size();
        Shift::gfx::StagingBuffer stagingBufferIndex{device, indexBufferSize};
        stagingBufferIndex.Fill(indices.data(), indexBufferSize);
        buffer.CopyBuffer(stagingBufferIndex.Get(), m_indexBuffer->Get(), indexBufferSize);

        buffer.EndCommandBuffer();
        buffer.SubmitAndWait();
    }
} // shift::gfx