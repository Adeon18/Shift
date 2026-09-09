//
// Created by otrush on 8/15/2026.
//

#ifndef SHIFT_IMODELLOADER_HPP
#define SHIFT_IMODELLOADER_HPP

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Shift {
    //! No idx - this
    inline constexpr uint32_t MODEL_INDEX_NONE = std::numeric_limits<uint32_t>::max();

    enum class ETextureColorSpace : uint8_t {
        SRGB,
        Linear
    };

    //! When materials don't have textures for stuff, these are placeholders
    enum class ETexturePlaceholder : uint8_t {
        //! base color, emissive
        WhiteSRGB,
        //! ORM
        WhiteLinear,
        //! normal: (0,0,1) in tangent space
        FlatNormal,
        Count
    };

    //! One material slot's texture. An empty id means the slot is unused
    struct TextureRef {
        std::string id;
        ETextureColorSpace colorSpace = ETextureColorSpace::Linear;
        ETexturePlaceholder placeholderKind = ETexturePlaceholder::WhiteLinear;

        [[nodiscard]] bool IsSet() const { return !id.empty(); }
    };

    //! An axis-aligned box plus the sphere that feeds ObjectData::boundsSphere
    struct Bounds {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
        //! xyz = center, w = radius
        glm::vec4 sphere{0.0f};

        [[nodiscard]] bool Contains(const glm::vec3& p) const {
            return p.x >= min.x && p.y >= min.y && p.z >= min.z
                && p.x <= max.x && p.y <= max.y && p.z <= max.z;
        }
    };

    //! The four parallel vertex streams plus the indices that address them.
    //! All four streams always hold the same number of elements, which should not break future features
    struct VertexStreams {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        //! w = the bitangent sign: bitangent = cross(normal, tangent.xyz) * tangent.w
        std::vector<glm::vec4> tangents;
        std::vector<glm::vec2> uvs;
        std::vector<uint32_t> indices;

        [[nodiscard]] uint32_t GetVertexCount() const { return static_cast<uint32_t>(positions.size()); }
        [[nodiscard]] uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }
        [[nodiscard]] uint32_t GetTriangleCount() const { return GetIndexCount() / 3; }

        //! Are they all the same size?
        [[nodiscard]] bool AreStreamsLockstep() const {
            const size_t count = positions.size();
            return normals.size() == count && tangents.size() == count && uvs.size() == count;
        }

        void Clear() {
            positions.clear();
            normals.clear();
            tangents.clear();
            uvs.clear();
            indices.clear();
        }
    };

    //! Mesh can have multiple materials, submesh is one material
    struct SubmeshDesc {
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        uint32_t materialIndex = MODEL_INDEX_NONE;
        Bounds bounds;
    };

    //! One glTF mesh
    struct MeshData {
        std::string name;
        VertexStreams streams;
        std::vector<SubmeshDesc> submeshes;
        Bounds bounds;
    };

    struct MaterialDesc {
        std::string name;

        glm::vec4 baseColorFactor{1.0f};
        glm::vec3 emissiveFactor{0.0f};
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        float alphaCutoff = 0.5f;

        //! MaterialData::flags bit0: selects the alpha-test pipeline variant
        bool alphaTest = false;
        bool doubleSided = false;

        //! Slot default properties are here
        TextureRef baseColor{.colorSpace = ETextureColorSpace::SRGB,
                             .placeholderKind = ETexturePlaceholder::WhiteSRGB};
        TextureRef normal{.colorSpace = ETextureColorSpace::Linear,
                          .placeholderKind = ETexturePlaceholder::FlatNormal};
        //! Oclussion Roughness Matellic. When supporting non-gltf formats imma die
        TextureRef orm{.colorSpace = ETextureColorSpace::Linear,
                       .placeholderKind = ETexturePlaceholder::WhiteLinear};
        TextureRef emissive{.colorSpace = ETextureColorSpace::SRGB,
                            .placeholderKind = ETexturePlaceholder::WhiteSRGB};
    };

    //! Scene hierarchny node for now
    struct NodeDesc {
        std::string name;
        uint32_t parent = MODEL_INDEX_NONE;
        uint32_t meshIndex = MODEL_INDEX_NONE;

        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
    };

    struct ModelData {
        //! The file this came from. Texture ids are relative to its directory
        std::string sourcePath;
        std::vector<MeshData> meshes;
        std::vector<MaterialDesc> materials;
        std::vector<NodeDesc> nodes;
    };

    class IModelLoader {
    public:
        virtual ~IModelLoader() = default;

        //! Loads and fully processes a model
        //! Generates tangents if there are none
        //! Does not upload to GPU
        virtual std::optional<ModelData> LoadFromFile(const std::string& path) = 0;
    };
}

#endif //SHIFT_IMODELLOADER_HPP
