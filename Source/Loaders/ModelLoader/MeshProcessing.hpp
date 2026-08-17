//
// Created by otrush on 8/15/2026.
//

#ifndef SHIFT_MESHPROCESSING_HPP
#define SHIFT_MESHPROCESSING_HPP

#include "Loaders/ModelLoader/IModelLoader.hpp"

//! Process a mesh at load with zeux's meshoptimizer
namespace Shift::MeshProcessing {
    //! What ProcessPrimitive had to do, for logs, mostly
    struct ProcessStats {
        bool generatedNormals = false;
        bool generatedTangents = false;
        //! No UVs in the source, so tangents are an arbitrary basis rather than the real thing
        bool usedFallbackTangents = false;
        uint32_t verticesBefore = 0;
        uint32_t verticesAfter = 0;
        uint32_t triangleCount = 0;
    };

    //! Smooth, area-weighted normals from the index buffer. Does nothing if normals are present.
    bool GenerateNormalsIfMissing(VertexStreams& streams);

    //! If mesh has no UV -> can't generate tangents, so we just build a default basis
    void FillFallbackTangents(VertexStreams& streams);

    //! MikkTSpace tangents for a mesh that has none. Requires normals and UVs to be present and
    //! all four streams lockstep; returns false if UVs are missing.
    bool GenerateTangentsIfMissing(VertexStreams& streams);

    //! Mikktspace requires vertex per index
    void Unindex(VertexStreams& streams);

    //! Restore the index buffer back
    uint32_t WeldVertices(VertexStreams& streams);

    //! meshoptimizer's vertex cache and vertex fetch reordering
    void OptimizeForGPU(VertexStreams& streams);

    //! AABB plus the sphere that ends up in ObjectData::boundsSphere. The radius is measured
    //! against the real points, not the box diagonal, so it is not needlessly loose
    [[nodiscard]] Bounds ComputeBounds(const std::vector<glm::vec3>& positions, uint32_t first, uint32_t count);

    //! The full CPU-side treatment of on primitive in order:
    //!   1. normals, because tangent generation consumes them
    //!   2. tangents, which may "re-weld" vertices and therefore renumber every vertex
    //!   3. cache/fetch optimization, which renumbers them again
    bool ProcessPrimitive(VertexStreams& streams, ProcessStats* outStats);
}

#endif //SHIFT_MESHPROCESSING_HPP
