#include <doctest/doctest.h>

#include <cmath>
#include <string>
#include <unordered_map>

#include "Loaders/ModelLoader/GltfLoader.hpp"
#include "Loaders/ModelLoader/MeshProcessing.hpp"
#include "Utility/UtilStandard.hpp"

using Shift::GltfLoader;
using Shift::ModelData;
using Shift::VertexStreams;
namespace MP = Shift::MeshProcessing;

//! Note what these cases deliberately do NOT check: unit length, finiteness and w = +/-1 are all
//! true of the arbitrary fallback basis as well, so on their own they would go green against a
//! function that does nothing. Every case here compares the tangent against the UV mapping it is
//! supposed to describe - that is the only property that separates a real tangent from a plausible
//! looking one.

namespace {
    std::string BoxNoTangentPath() {
        return Shift::Util::GetShiftRoot() + "Assets/Test/BoxNoTangent.gltf";
    }

    std::string BoxPath() {
        return Shift::Util::GetShiftRoot() + "Assets/Test/Box.gltf";
    }

    VertexStreams MakeQuadWithoutTangents() {
        VertexStreams streams;
        streams.positions = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
        streams.normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
        //! u grows with +X, v grows DOWNWARD as in glTF texture space
        streams.uvs = {{0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}};
        streams.indices = {0, 1, 2, 0, 2, 3};
        return streams;
    }

    //! The tangent the UV mapping implies for one triangle: the direction in which u grows
    glm::vec3 AnalyticTangent(const VertexStreams& streams, size_t triangle) {
        const uint32_t i0 = streams.indices[triangle * 3];
        const uint32_t i1 = streams.indices[triangle * 3 + 1];
        const uint32_t i2 = streams.indices[triangle * 3 + 2];

        const glm::vec3 edge1 = streams.positions[i1] - streams.positions[i0];
        const glm::vec3 edge2 = streams.positions[i2] - streams.positions[i0];
        const glm::vec2 duv1 = streams.uvs[i1] - streams.uvs[i0];
        const glm::vec2 duv2 = streams.uvs[i2] - streams.uvs[i0];

        const float determinant = duv1.x * duv2.y - duv2.x * duv1.y;
        if (std::abs(determinant) < 1e-12f) return glm::vec3{0.0f};
        return glm::normalize((edge1 * duv2.y - edge2 * duv1.y) / determinant);
    }

    //! Identifies a vertex across two assets that share geometry but not vertex order.
    //!
    //! The NORMAL is part of the key and has to be: position + uv is ambiguous on this cube.
    //! Corner (-0.5,-0.5,-0.5) carries uv (0,1) on BOTH the -X and the -Y face, since every face
    //! lays out its own 0..1 uv square the same way - so a position+uv key silently matched a
    //! neighbouring face's tangent and reported dot == 0 (perpendicular, i.e. the wrong face's
    //! tangent, not a wrong tangent)
    std::string VertexKey(const glm::vec3& position, const glm::vec2& uv, const glm::vec3& normal) {
        auto round = [](float value) { return std::to_string(std::lround(value * 1000.0f)); };
        return round(position.x) + "_" + round(position.y) + "_" + round(position.z)
             + "|" + round(uv.x) + "_" + round(uv.y)
             + "|" + round(normal.x) + "_" + round(normal.y) + "_" + round(normal.z);
    }
}

TEST_SUITE("TangentGeneration") {

TEST_CASE("A quad's tangent points along +U with glTF's bitangent sign") {
    VertexStreams streams = MakeQuadWithoutTangents();

    REQUIRE(MP::GenerateTangentsIfMissing(streams));
    REQUIRE(streams.AreStreamsLockstep());
    REQUIRE(streams.GetTriangleCount() == 2);

    for (size_t i = 0; i < streams.tangents.size(); ++i) {
        const glm::vec4 tangent = streams.tangents[i];
        CHECK(tangent.x == doctest::Approx(1.0f).epsilon(1e-3));
        CHECK(tangent.y == doctest::Approx(0.0f).epsilon(1e-3));
        CHECK(tangent.z == doctest::Approx(0.0f).epsilon(1e-3));
        //! glTF: bitangent = cross(normal, tangent.xyz) * w. cross(+Z, +X) is +Y, while v grows
        //! toward -Y here, so the only sign that reconstructs this mapping is -1
        CHECK(tangent.w == doctest::Approx(-1.0f));
    }
}

TEST_CASE("The unindex/generate/re-weld round trip preserves the mesh") {
    GltfLoader loader;
    const std::optional<ModelData> model = loader.LoadFromFile(BoxNoTangentPath());
    REQUIRE(model.has_value());
    REQUIRE(model->meshes.size() == 1);

    const VertexStreams& streams = model->meshes[0].streams;

    //! 36 corners come out of the unindex; each of the cube's 24 vertices is shared by two
    //! triangles of the SAME face, so tangent space agrees there and the weld collapses them back.
    //! A number above 24 means the weld found a difference where there is none
    CHECK(streams.GetVertexCount() == 24);
    CHECK(streams.GetIndexCount() == 36);
    CHECK(streams.GetTriangleCount() == 12);
    CHECK(streams.AreStreamsLockstep());
}

TEST_CASE("Generated tangents describe the asset's UV mapping") {
    GltfLoader loader;
    const std::optional<ModelData> model = loader.LoadFromFile(BoxNoTangentPath());
    REQUIRE(model.has_value());
    REQUIRE(model->meshes.size() == 1);

    const VertexStreams& streams = model->meshes[0].streams;
    REQUIRE(streams.tangents.size() == streams.positions.size());

    for (size_t triangle = 0; triangle < streams.GetTriangleCount(); ++triangle) {
        const glm::vec3 expected = AnalyticTangent(streams, triangle);
        REQUIRE(glm::length(expected) == doctest::Approx(1.0f).epsilon(1e-3));

        for (size_t corner = 0; corner < 3; ++corner) {
            const uint32_t vertex = streams.indices[triangle * 3 + corner];
            const glm::vec4 tangent = streams.tangents[vertex];
            const glm::vec3 direction{tangent};

            CHECK(std::isfinite(tangent.w));
            CHECK(glm::length(direction) == doctest::Approx(1.0f).epsilon(1e-3));
            //! The real check: it has to point the way u grows on this face. An arbitrary
            //! orthonormal basis around the normal passes every other assertion and fails this one
            CHECK(glm::dot(direction, expected) > 0.99f);
            //! Perpendicular to the normal, or the shader's TBN is not orthonormal
            CHECK(glm::dot(direction, streams.normals[vertex]) == doctest::Approx(0.0f).epsilon(1e-3));
            CHECK(std::abs(tangent.w) == doctest::Approx(1.0f));
        }
    }
}

TEST_CASE("Generated tangents match the ones the authored asset ships") {
    GltfLoader loader;
    const std::optional<ModelData> authored = loader.LoadFromFile(BoxPath());
    const std::optional<ModelData> generated = loader.LoadFromFile(BoxNoTangentPath());
    REQUIRE(authored.has_value());
    REQUIRE(generated.has_value());
    REQUIRE(authored->meshes.size() == 1);
    REQUIRE(generated->meshes.size() == 1);

    //! Box.gltf carries tangents computed offline from the same UV layout, and the loader passes
    //! them through untouched. So the two paths - authored and generated - have to agree, which
    //! is a far stronger statement than either being self-consistent
    const VertexStreams& authoredStreams = authored->meshes[0].streams;
    std::unordered_map<std::string, glm::vec4> authoredTangents;
    for (size_t i = 0; i < authoredStreams.positions.size(); ++i) {
        authoredTangents.emplace(VertexKey(authoredStreams.positions[i], authoredStreams.uvs[i], authoredStreams.normals[i]),
                                 authoredStreams.tangents[i]);
    }

    const VertexStreams& generatedStreams = generated->meshes[0].streams;
    size_t matched = 0;
    for (size_t i = 0; i < generatedStreams.positions.size(); ++i) {
        const auto found = authoredTangents.find(VertexKey(generatedStreams.positions[i], generatedStreams.uvs[i],
                                                           generatedStreams.normals[i]));
        if (found == authoredTangents.end()) continue;

        ++matched;
        const glm::vec4 expected = found->second;
        const glm::vec4 actual = generatedStreams.tangents[i];
        CHECK(glm::dot(glm::vec3{actual}, glm::vec3{expected}) > 0.99f);
        CHECK(actual.w == doctest::Approx(expected.w));
    }

    //! Both cubes are the same 24 vertices, so nothing may go unmatched
    CHECK(matched == generatedStreams.positions.size());
}

}
