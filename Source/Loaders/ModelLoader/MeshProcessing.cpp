//
// Created by otrush on 8/15/2026.
//
#include "Loaders/ModelLoader/MeshProcessing.hpp"

#include <algorithm>
#include <cmath>

#include <meshoptimizer.h>

#include "Utility/Assertions.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::MeshProcessing {

    namespace {
        //! Applies a meshoptimizer remap table to the index buffer and all four vertex streams.
        void ApplyRemap(VertexStreams& streams, const std::vector<uint32_t>& remap, size_t uniqueCount) {
            const size_t sourceVertexCount = streams.positions.size();
            //! meshoptimizer's convention: a null index buffer means "unindexed", and then the
            //! index count is the vertex count
            const bool wasIndexed = !streams.indices.empty();
            const size_t indexCount = wasIndexed ? streams.indices.size() : sourceVertexCount;

            std::vector<uint32_t> newIndices(indexCount);
            meshopt_remapIndexBuffer(newIndices.data(), wasIndexed ? streams.indices.data() : nullptr,
                                     indexCount, remap.data());

            std::vector<glm::vec3> positions(uniqueCount);
            std::vector<glm::vec3> normals(uniqueCount);
            std::vector<glm::vec4> tangents(uniqueCount);
            std::vector<glm::vec2> uvs(uniqueCount);

            meshopt_remapVertexBuffer(positions.data(), streams.positions.data(), sourceVertexCount,
                                      sizeof(glm::vec3), remap.data());
            meshopt_remapVertexBuffer(normals.data(), streams.normals.data(), sourceVertexCount,
                                      sizeof(glm::vec3), remap.data());
            meshopt_remapVertexBuffer(tangents.data(), streams.tangents.data(), sourceVertexCount,
                                      sizeof(glm::vec4), remap.data());
            meshopt_remapVertexBuffer(uvs.data(), streams.uvs.data(), sourceVertexCount,
                                      sizeof(glm::vec2), remap.data());

            streams.indices = std::move(newIndices);
            streams.positions = std::move(positions);
            streams.normals = std::move(normals);
            streams.tangents = std::move(tangents);
            streams.uvs = std::move(uvs);
        }

        //! Any unit vector perpendicular to n
        glm::vec3 PerpendicularTo(const glm::vec3& n) {
            const glm::vec3 axis = (std::abs(n.z) < 0.999f) ? glm::vec3{0.0f, 0.0f, 1.0f}
                                                            : glm::vec3{1.0f, 0.0f, 0.0f};
            const glm::vec3 perpendicular = glm::cross(axis, n);
            const float length = glm::length(perpendicular);
            return (length > 1e-12f) ? perpendicular / length : glm::vec3{1.0f, 0.0f, 0.0f};
        }
    }

    bool GenerateNormalsIfMissing(VertexStreams& streams) {
        if (!streams.normals.empty()) return true;

        Check(Warning, !streams.indices.empty(), "Cannot derive normals without an index buffer");

        const size_t vertexCount = streams.positions.size();
        streams.normals.assign(vertexCount, glm::vec3{0.0f});

        for (size_t i = 0; i + 2 < streams.indices.size(); i += 3) {
            const uint32_t i0 = streams.indices[i];
            const uint32_t i1 = streams.indices[i + 1];
            const uint32_t i2 = streams.indices[i + 2];

            //! multiple vertices consists of multiple triangles so we accumulate the nortmal and weight it by tri size (not normalize) for a better smooth look
            const glm::vec3 faceNormal = glm::cross(streams.positions[i1] - streams.positions[i0],
                                                    streams.positions[i2] - streams.positions[i0]);
            streams.normals[i0] += faceNormal;
            streams.normals[i1] += faceNormal;
            streams.normals[i2] += faceNormal;
        }

        for (glm::vec3& normal : streams.normals) {
            const float length = glm::length(normal);
            //! Up vector is the normal if the normal is degenerate
            normal = (length > 1e-12f) ? normal / length : glm::vec3{0.0f, 1.0f, 0.0f};
        }

        LogInfo("MeshProcessing: generated smooth normals for {} vertices", vertexCount);
        return true;
    }

    void FillFallbackTangents(VertexStreams& streams) {
        const bool hasNormals = streams.normals.size() == streams.positions.size();
        streams.tangents.resize(streams.positions.size());
        for (size_t i = 0; i < streams.tangents.size(); ++i) {
            streams.tangents[i] = hasNormals ? glm::vec4{PerpendicularTo(streams.normals[i]), 1.0f}
                                             : glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
        }
    }

    void Unindex(VertexStreams& streams) {
        if (streams.indices.empty()) return;

        const size_t cornerCount = streams.indices.size();

        const bool hasNormals = !streams.normals.empty();
        const bool hasTangents = !streams.tangents.empty();
        const bool hasUVs = !streams.uvs.empty();

        std::vector<glm::vec3> positions(cornerCount);
        std::vector<glm::vec3> normals(hasNormals ? cornerCount : 0);
        std::vector<glm::vec4> tangents(hasTangents ? cornerCount : 0);
        std::vector<glm::vec2> uvs(hasUVs ? cornerCount : 0);
        std::vector<uint32_t> identity(cornerCount);

        for (size_t corner = 0; corner < cornerCount; ++corner) {
            const uint32_t source = streams.indices[corner];
            positions[corner] = streams.positions[source];
            if (hasNormals) normals[corner] = streams.normals[source];
            if (hasTangents) tangents[corner] = streams.tangents[source];
            if (hasUVs) uvs[corner] = streams.uvs[source];
            identity[corner] = static_cast<uint32_t>(corner);
        }

        streams.positions = std::move(positions);
        streams.normals = std::move(normals);
        streams.tangents = std::move(tangents);
        streams.uvs = std::move(uvs);
        streams.indices = std::move(identity);
    }

    uint32_t WeldVertices(VertexStreams& streams) {
        const size_t vertexCount = streams.positions.size();
        if (vertexCount == 0) return 0;

        if (!streams.AreStreamsLockstep()) {
            LogError("MeshProcessing: refusing to weld, the four vertex streams are not the same length");
            return static_cast<uint32_t>(vertexCount);
        }

        const meshopt_Stream sourceStreams[4] = {
            {streams.positions.data(), sizeof(glm::vec3), sizeof(glm::vec3)},
            {streams.normals.data(), sizeof(glm::vec3), sizeof(glm::vec3)},
            {streams.tangents.data(), sizeof(glm::vec4), sizeof(glm::vec4)},
            {streams.uvs.data(), sizeof(glm::vec2), sizeof(glm::vec2)},
        };

        const bool isIndexed = !streams.indices.empty();
        const size_t indexCount = isIndexed ? streams.indices.size() : vertexCount;

        std::vector<uint32_t> remap(vertexCount);
        const size_t uniqueCount = meshopt_generateVertexRemapMulti(
            remap.data(), isIndexed ? streams.indices.data() : nullptr, indexCount, vertexCount,
            sourceStreams, 4);

        ApplyRemap(streams, remap, uniqueCount);
        return static_cast<uint32_t>(uniqueCount);
    }

    void OptimizeForGPU(VertexStreams& streams) {
        const size_t vertexCount = streams.positions.size();
        const size_t indexCount = streams.indices.size();
        if (vertexCount == 0 || indexCount == 0) return;

        if (!streams.AreStreamsLockstep()) {
            LogError("MeshProcessing: refusing to optimize, the four vertex streams are not the same length");
            return;
        }

        //! Optimize the vertex cache reuse by reordering triangles
        meshopt_optimizeVertexCache(streams.indices.data(), streams.indices.data(), indexCount, vertexCount);

        std::vector<uint32_t> remap(vertexCount);
        //! Optimize for locality of vertex fetch during vertex pulling
        const size_t uniqueCount = meshopt_optimizeVertexFetchRemap(remap.data(), streams.indices.data(),
                                                                    indexCount, vertexCount);
        ApplyRemap(streams, remap, uniqueCount);
    }

    Bounds ComputeBounds(const std::vector<glm::vec3>& positions, uint32_t first, uint32_t count) {
        Bounds bounds;
        if (count == 0 || first >= positions.size()) return bounds;

        const size_t last = std::min<size_t>(positions.size(), static_cast<size_t>(first) + count);

        bounds.min = positions[first];
        bounds.max = positions[first];
        for (size_t i = first; i < last; ++i) {
            bounds.min = glm::min(bounds.min, positions[i]);
            bounds.max = glm::max(bounds.max, positions[i]);
        }

        const glm::vec3 center = (bounds.min + bounds.max) * 0.5f;
        float radiusSquared = 0.0f;
        for (size_t i = first; i < last; ++i) {
            const glm::vec3 offset = positions[i] - center;
            radiusSquared = std::max(radiusSquared, glm::dot(offset, offset));
        }

        bounds.sphere = glm::vec4{center, std::sqrt(radiusSquared)};
        return bounds;
    }

    bool ProcessPrimitive(VertexStreams& streams, ProcessStats* outStats) {
        ProcessStats stats;
        stats.verticesBefore = streams.GetVertexCount();

        Check(Warning, !streams.positions.empty(), "A primitive with no positions cannot be processed");
        //! Zero is a multiple of 3, so the check below passes an empty index buffer. Left to run, it
        //! produces a submesh with indexCount == 0 plus dead vertices in the merged streams, and
        //! reports the tangent step as the failure on the way there
        Check(Warning, !streams.indices.empty(), "A primitive with no indices draws nothing");
        Check(Warning, streams.indices.size() % 3 == 0, "Index count is not a multiple of 3 - not a triangle list");
        for (const uint32_t index : streams.indices) {
            Check(Warning, index < streams.positions.size(), "An index points past the end of the vertex streams");
        }

        //! Check for position stream mismatch with vtx stream
        const size_t vertexCount = streams.positions.size();
        const bool suppliedStreamsSized =
            (streams.normals.empty() || streams.normals.size() == vertexCount)
            && (streams.tangents.empty() || streams.tangents.size() == vertexCount)
            && (streams.uvs.empty() || streams.uvs.size() == vertexCount);
        Check(Warning, suppliedStreamsSized, "A supplied vertex stream is shorter than the position stream");

        stats.generatedNormals = streams.normals.empty();
        CheckExit(GenerateNormalsIfMissing(streams));

        const bool hasUVs = !streams.uvs.empty();
        if (!hasUVs) streams.uvs.assign(streams.positions.size(), glm::vec2{0.0f});

        if (streams.tangents.empty()) {
            const bool generated = hasUVs && GenerateTangentsIfMissing(streams);
            if (!generated) {
                if (hasUVs) {
                    LogError("MeshProcessing: tangent generation failed despite UVs being present, "
                             "falling back to an arbitrary basis - normal mapping will be wrong");
                } else {
                    LogWarn("MeshProcessing: no UVs, so tangent space is undefined - filling an "
                            "arbitrary basis");
                }
                FillFallbackTangents(streams);
            }
            stats.generatedTangents = generated;
            stats.usedFallbackTangents = !generated;
        }

        Check(Warning, streams.AreStreamsLockstep(), "Vertex streams came out of processing at different lengths");

        OptimizeForGPU(streams);

        stats.verticesAfter = streams.GetVertexCount();
        stats.triangleCount = streams.GetTriangleCount();
        if (outStats) { *outStats = stats; }
        return true;
    }
}
