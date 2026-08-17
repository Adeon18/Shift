#include <doctest/doctest.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "Loaders/ModelLoader/MeshProcessing.hpp"

using Shift::Bounds;
using Shift::VertexStreams;
namespace MP = Shift::MeshProcessing;

//! The format-neutral half of the mesh pipeline: unindex, weld, optimize, normals, bounds.
//! Tangent generation itself lives in Test_TangentGeneration.cpp, because it is a separate piece
//! of work with a separate author.
//!
//! The load-bearing property behind most of these cases is that the four streams stay LOCKSTEP.
//! One BufferRangeAllocator range has to address the same [first, count) in four merged buffers
//! whose byte strides differ (12/12/16/8), so a step that renumbers one stream and not the other
//! three is not a wrong picture, it is three wrong pictures.

namespace {
    //! Bit-exact, because every step here MOVES vertices rather than computing new ones - a key
    //! that rounded would hide precisely the corruption worth catching
    std::string PositionKey(const glm::vec3& position) {
        uint32_t bits[3];
        std::memcpy(bits, &position, sizeof(bits));
        return std::to_string(bits[0]) + "_" + std::to_string(bits[1]) + "_" + std::to_string(bits[2]);
    }

    //! One triangle as a winding-preserving key: rotated to start at its smallest vertex key, so
    //! reordering triangles or renumbering vertices does not change it, but flipping one does
    std::string TriangleKey(const VertexStreams& streams, size_t triangle) {
        std::string keys[3] = {
            PositionKey(streams.positions[streams.indices[triangle * 3]]),
            PositionKey(streams.positions[streams.indices[triangle * 3 + 1]]),
            PositionKey(streams.positions[streams.indices[triangle * 3 + 2]]),
        };
        const size_t start = std::distance(std::begin(keys), std::min_element(std::begin(keys), std::end(keys)));
        return keys[start] + "|" + keys[(start + 1) % 3] + "|" + keys[(start + 2) % 3];
    }

    std::vector<std::string> TriangleKeys(const VertexStreams& streams) {
        std::vector<std::string> keys;
        for (size_t triangle = 0; triangle < streams.GetTriangleCount(); ++triangle) {
            keys.push_back(TriangleKey(streams, triangle));
        }
        std::sort(keys.begin(), keys.end());
        return keys;
    }

    //! A unit quad in the XY plane facing +Z, two triangles, four vertices.
    //! V grows downward as it does in glTF texture space, which is what makes the correct
    //! bitangent sign -1 here (and on the Box.gltf assets, generated with the same convention)
    VertexStreams MakeQuad() {
        VertexStreams streams;
        streams.positions = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        streams.normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
        streams.uvs = {{0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}};
        streams.indices = {0, 1, 2, 0, 2, 3};
        return streams;
    }

    void FillTangents(VertexStreams& streams, const glm::vec4& tangent) {
        streams.tangents.assign(streams.positions.size(), tangent);
    }
}

TEST_SUITE("MeshProcessing") {

TEST_CASE("Unindex expands to one vertex per corner and keeps the triangles") {
    VertexStreams streams = MakeQuad();
    FillTangents(streams, {1.0f, 0.0f, 0.0f, 1.0f});
    const std::vector<std::string> before = TriangleKeys(streams);

    MP::Unindex(streams);

    CHECK(streams.GetVertexCount() == 6);
    CHECK(streams.GetIndexCount() == 6);
    CHECK(streams.AreStreamsLockstep());
    //! The identity index buffer is kept so triangle count still reads correctly mid-pipeline
    CHECK(streams.indices[0] == 0);
    CHECK(streams.indices[5] == 5);
    CHECK(TriangleKeys(streams) == before);
}

TEST_CASE("Welding merges corners that agree in all four streams") {
    VertexStreams streams = MakeQuad();
    FillTangents(streams, {1.0f, 0.0f, 0.0f, 1.0f});
    const std::vector<std::string> before = TriangleKeys(streams);

    MP::Unindex(streams);
    REQUIRE(streams.GetVertexCount() == 6);

    const uint32_t welded = MP::WeldVertices(streams);

    //! The two corners the triangles share carry identical position/normal/uv/tangent, so 6 go
    //! back down to 4 - this round trip is the whole reason tangent generation can unindex freely
    CHECK(welded == 4);
    CHECK(streams.GetVertexCount() == 4);
    CHECK(streams.GetIndexCount() == 6);
    CHECK(streams.AreStreamsLockstep());
    CHECK(TriangleKeys(streams) == before);
}

TEST_CASE("Welding keeps vertices that differ ONLY in tangent apart") {
    VertexStreams streams = MakeQuad();
    FillTangents(streams, {1.0f, 0.0f, 0.0f, 1.0f});
    MP::Unindex(streams);

    //! Exactly what MikkTSpace produces at a UV seam: same position, same normal, same uv,
    //! different tangent space. A weld that compared positions only would glue the seam shut
    //! and undo the split
    streams.tangents[3] = glm::vec4{0.0f, 1.0f, 0.0f, -1.0f};

    const uint32_t welded = MP::WeldVertices(streams);

    CHECK(welded == 5);
    CHECK(streams.GetIndexCount() == 6);
    CHECK(streams.AreStreamsLockstep());
}

TEST_CASE("Missing normals are generated, present ones are left alone") {
    VertexStreams streams = MakeQuad();
    streams.normals.clear();

    REQUIRE(MP::GenerateNormalsIfMissing(streams));

    REQUIRE(streams.normals.size() == 4);
    for (const glm::vec3& normal : streams.normals) {
        CHECK(normal.z == doctest::Approx(1.0f));
        CHECK(glm::length(normal) == doctest::Approx(1.0f));
    }

    //! Second call is a no-op: an asset that shipped normals keeps the ones it authored
    streams.normals[0] = glm::vec3{0.0f, 1.0f, 0.0f};
    REQUIRE(MP::GenerateNormalsIfMissing(streams));
    CHECK(streams.normals[0].y == doctest::Approx(1.0f));
}

TEST_CASE("GPU optimization reorders without changing the triangles") {
    VertexStreams streams = MakeQuad();
    FillTangents(streams, {1.0f, 0.0f, 0.0f, 1.0f});
    const std::vector<std::string> before = TriangleKeys(streams);

    MP::OptimizeForGPU(streams);

    CHECK(streams.GetIndexCount() == 6);
    CHECK(streams.AreStreamsLockstep());
    CHECK(TriangleKeys(streams) == before);
}

TEST_CASE("Bounds hold every position, box and sphere alike") {
    VertexStreams streams = MakeQuad();

    const Bounds bounds = MP::ComputeBounds(streams.positions, 0, streams.GetVertexCount());

    CHECK(bounds.min.x == doctest::Approx(0.0f));
    CHECK(bounds.max.x == doctest::Approx(1.0f));
    for (const glm::vec3& position : streams.positions) {
        CHECK(bounds.Contains(position));
        const float distance = glm::length(position - glm::vec3{bounds.sphere});
        CHECK(distance <= bounds.sphere.w + 1e-5f);
    }

    //! A submesh's bounds are a window into the mesh's stream, so the range has to be honoured
    const Bounds firstTwo = MP::ComputeBounds(streams.positions, 0, 2);
    CHECK(firstTwo.max.y == doctest::Approx(0.0f));
}

TEST_CASE("Processing refuses malformed geometry instead of walking off the end") {
    VertexStreams outOfRange = MakeQuad();
    outOfRange.indices[2] = 99;
    CHECK_FALSE(MP::ProcessPrimitive(outOfRange, nullptr));

    VertexStreams notTriangles = MakeQuad();
    notTriangles.indices.pop_back();
    CHECK_FALSE(MP::ProcessPrimitive(notTriangles, nullptr));

    VertexStreams shortStream = MakeQuad();
    shortStream.normals.pop_back();
    CHECK_FALSE(MP::ProcessPrimitive(shortStream, nullptr));
}

TEST_CASE("A mesh with no UVs still comes out with a usable basis") {
    VertexStreams streams = MakeQuad();
    streams.uvs.clear();

    MP::ProcessStats stats;
    REQUIRE(MP::ProcessPrimitive(streams, &stats));

    //! Tangent space is undefined without UVs, so this is explicitly NOT correct normal mapping -
    //! it is a basis that keeps the shader's TBN non-singular, and it says so in the stats
    CHECK(stats.usedFallbackTangents);
    CHECK_FALSE(stats.generatedTangents);
    CHECK(streams.AreStreamsLockstep());
    for (size_t i = 0; i < streams.tangents.size(); ++i) {
        const glm::vec3 tangent{streams.tangents[i]};
        CHECK(glm::length(tangent) == doctest::Approx(1.0f));
        CHECK(glm::dot(tangent, streams.normals[i]) == doctest::Approx(0.0f).epsilon(1e-4));
    }
}

}
