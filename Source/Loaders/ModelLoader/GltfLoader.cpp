//
// Created by otrush on 8/15/2026.
//
#include "Loaders/ModelLoader/GltfLoader.hpp"

#include <filesystem>
#include <variant>

#include <glm/glm.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>

#include "Loaders/ModelLoader/MeshProcessing.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift {

    namespace {
        template<typename StringT>
        std::string ToStdString(const StringT& str) {
            return std::string(str.data(), str.size());
        }

        //! Get Image ID
        std::string ResolveTextureID(const fastgltf::Asset& asset, size_t textureIndex) {
            if (textureIndex >= asset.textures.size()) {
                LogWarn("GltfLoader: material references texture {} which does not exist", textureIndex);
                return {};
            }

            const fastgltf::Texture& texture = asset.textures[textureIndex];
            if (!texture.imageIndex.has_value()) {
                LogWarn("GltfLoader: texture {} has no plain image source", textureIndex);
                return {};
            }

            //! Image is the actual data, texture is image + sampker
            const size_t imageIndex = texture.imageIndex.value();
            if (imageIndex >= asset.images.size()) {
                LogWarn("GltfLoader: texture {} names image {} which does not exist", textureIndex, imageIndex);
                return {};
            }

            const fastgltf::Image& image = asset.images[imageIndex];
            const auto* idSource = std::get_if<fastgltf::sources::URI>(&image.data);
            if (!idSource) {
                LogWarn("GltfLoader: image of texture {} is embedded, which the texture pipeline "
                        "does not read yet", textureIndex);
                return {};
            }
            if (idSource->uri.isDataUri()) {
                LogWarn("GltfLoader: image of texture {} is a data URI, skipped", textureIndex);
                return {};
            }

            return std::string(idSource->uri.string());
        }

        void WarnOnUnsupportedTexCoord(const fastgltf::TextureInfo& info, std::string_view slot) {
            if (info.texCoordIndex != 0) {
                LogWarn("GltfLoader: the {} slot uses TEXCOORD_{}, but only TEXCOORD_0 is read", slot,
                        info.texCoordIndex);
            }
        }

        void FillTextureRef(const fastgltf::Asset& asset, const fastgltf::Optional<fastgltf::TextureInfo>& info,
                            ETextureColorSpace colorSpace, ETexturePlaceholder placeholderKind, std::string_view slot, TextureRef* outRef) {
            //! TODO: [DESIGN] Should the placeholder be set here?
            outRef->colorSpace = colorSpace;
            outRef->placeholderKind = placeholderKind;

            if (!info.has_value()) return;
            WarnOnUnsupportedTexCoord(info.value(), slot);
            outRef->id = ResolveTextureID(asset, info.value().textureIndex);
        }

        MaterialDesc ConvertMaterial(const fastgltf::Asset& asset, const fastgltf::Material& source) {
            MaterialDesc material;
            material.name = ToStdString(source.name);

            const fastgltf::PBRData& pbr = source.pbrData;
            material.baseColorFactor = glm::vec4{static_cast<float>(pbr.baseColorFactor[0]),
                                                 static_cast<float>(pbr.baseColorFactor[1]),
                                                 static_cast<float>(pbr.baseColorFactor[2]),
                                                 static_cast<float>(pbr.baseColorFactor[3])};
            material.metallicFactor = static_cast<float>(pbr.metallicFactor);
            material.roughnessFactor = static_cast<float>(pbr.roughnessFactor);

            material.emissiveFactor = glm::vec3{static_cast<float>(source.emissiveFactor[0]),
                                                static_cast<float>(source.emissiveFactor[1]),
                                                static_cast<float>(source.emissiveFactor[2])};
            material.alphaCutoff = static_cast<float>(source.alphaCutoff);
            material.alphaTest = source.alphaMode == fastgltf::AlphaMode::Mask;
            material.doubleSided = source.doubleSided;

            //! We decide here on the color space
            FillTextureRef(asset, pbr.baseColorTexture, ETextureColorSpace::SRGB,
                           ETexturePlaceholder::WhiteSRGB, "base color", &material.baseColor);
            FillTextureRef(asset, pbr.metallicRoughnessTexture, ETextureColorSpace::Linear,
                           ETexturePlaceholder::WhiteLinear, "ORM", &material.orm);
            FillTextureRef(asset, source.emissiveTexture, ETextureColorSpace::SRGB,
                           ETexturePlaceholder::WhiteSRGB, "emissive", &material.emissive);

            //! FYI: Normal texture is a separate type in gltf
            material.normal.colorSpace = ETextureColorSpace::Linear;
            material.normal.placeholderKind = ETexturePlaceholder::FlatNormal;
            if (source.normalTexture.has_value()) {
                const fastgltf::NormalTextureInfo& normalInfo = source.normalTexture.value();
                WarnOnUnsupportedTexCoord(normalInfo, "normal");
                material.normal.id = ResolveTextureID(asset, normalInfo.textureIndex);
                //! We ignore scaled normal maps
                if (normalInfo.scale != 1.0f) {
                    LogWarn("GltfLoader: material '{}' scales its normal map by {}, which is not carried",
                            material.name, static_cast<float>(normalInfo.scale));
                }
            }

            //! If no ORM texture -> lose AO and keep Metallic Roighness as they come in same texture anyway
            if (source.occlusionTexture.has_value()) {
                const size_t occlusionIndex = source.occlusionTexture.value().textureIndex;
                const bool packedTogether = pbr.metallicRoughnessTexture.has_value()
                    && pbr.metallicRoughnessTexture.value().textureIndex == occlusionIndex;
                if (!packedTogether) {
                    LogWarn("GltfLoader: material '{}' has a separate occlusion texture; only the "
                            "metallic-roughness image fills the ORM slot", material.name);
                }
            }

            return material;
        }

        //! Read one attribute into glm stream, false of that stream (NORMAL, TANGENT) were not declared in primitive
        template<typename ElementT>
        bool ReadAttribute(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                           std::string_view name, fastgltf::AccessorType expectedType,
                           std::vector<ElementT>* outStream) {
            const auto* attribute = primitive.findAttribute(name);
            if (attribute == primitive.attributes.cend()) return false;

            if (attribute->accessorIndex >= asset.accessors.size()) {
                LogWarn("GltfLoader: attribute {} names an accessor that does not exist, skipped", name);
                return false;
            }

            const fastgltf::Accessor& accessor = asset.accessors[attribute->accessorIndex];
            if (accessor.type != expectedType) {
                LogWarn("GltfLoader: attribute {} has an unexpected accessor type, skipped", name);
                return false;
            }
            if (!accessor.bufferViewIndex.has_value() && !accessor.sparse.has_value()) {
                LogWarn("GltfLoader: attribute {} has neither a buffer view nor sparse data, skipped", name);
                return false;
            }

            outStream->resize(accessor.count);
            fastgltf::copyFromAccessor<ElementT>(asset, accessor, outStream->data());
            return true;
        }

        bool ReadPrimitiveStreams(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
                                  VertexStreams* outStreams) {
            if (!ReadAttribute(asset, primitive, "POSITION", fastgltf::AccessorType::Vec3,
                               &outStreams->positions)) {
                LogWarn("GltfLoader: a primitive has no POSITION attribute, skipped");
                return false;
            }

            ReadAttribute(asset, primitive, "NORMAL", fastgltf::AccessorType::Vec3, &outStreams->normals);
            ReadAttribute(asset, primitive, "TEXCOORD_0", fastgltf::AccessorType::Vec2, &outStreams->uvs);
            ReadAttribute(asset, primitive, "TANGENT", fastgltf::AccessorType::Vec4, &outStreams->tangents);

            //! Options::GenerateMeshIndices makes this hold even for a non-indexed primitive
            if (!primitive.indicesAccessor.has_value()) {
                LogWarn("GltfLoader: a primitive has no indices, skipped");
                return false;
            }
            if (primitive.indicesAccessor.value() >= asset.accessors.size()) {
                LogWarn("GltfLoader: a primitive names an index accessor that does not exist, skipped");
                return false;
            }

            const fastgltf::Accessor& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];
            if (indexAccessor.type != fastgltf::AccessorType::Scalar) {
                LogWarn("GltfLoader: a primitive's index accessor is not scalar, skipped");
                return false;
            }
            if (!indexAccessor.bufferViewIndex.has_value() && !indexAccessor.sparse.has_value()) {
                LogWarn("GltfLoader: a primitive's index accessor has neither a buffer view nor "
                        "sparse data, skipped");
                return false;
            }
            outStreams->indices.resize(indexAccessor.count);
            fastgltf::copyFromAccessor<uint32_t>(asset, indexAccessor, outStreams->indices.data());
            return true;
        }

        void AppendPrimitive(MeshData* mesh, const VertexStreams& primitive, uint32_t materialIndex) {
            const uint32_t vertexBase = mesh->streams.GetVertexCount();
            const uint32_t firstIndex = mesh->streams.GetIndexCount();

            //! Insert primitive and re-map indices
            VertexStreams& target = mesh->streams;
            target.positions.insert(target.positions.end(), primitive.positions.begin(), primitive.positions.end());
            target.normals.insert(target.normals.end(), primitive.normals.begin(), primitive.normals.end());
            target.tangents.insert(target.tangents.end(), primitive.tangents.begin(), primitive.tangents.end());
            target.uvs.insert(target.uvs.end(), primitive.uvs.begin(), primitive.uvs.end());

            target.indices.reserve(target.indices.size() + primitive.indices.size());
            for (const uint32_t index : primitive.indices) target.indices.push_back(index + vertexBase);

            SubmeshDesc submesh;
            submesh.firstIndex = firstIndex;
            submesh.indexCount = primitive.GetIndexCount();
            submesh.materialIndex = materialIndex;
            submesh.bounds = MeshProcessing::ComputeBounds(target.positions, vertexBase,
                                                           primitive.GetVertexCount());
            mesh->submeshes.push_back(submesh);
        }

        void ConvertNodeTransform(const fastgltf::Node& source, NodeDesc* outNode) {
            fastgltf::math::fvec3 translation{0.0f, 0.0f, 0.0f};
            fastgltf::math::fquat rotation{0.0f, 0.0f, 0.0f, 1.0f};
            fastgltf::math::fvec3 scale{1.0f, 1.0f, 1.0f};

            if (const auto* trs = std::get_if<fastgltf::TRS>(&source.transform)) {
                translation = trs->translation;
                rotation = trs->rotation;
                scale = trs->scale;
            } else if (const auto* matrix = std::get_if<fastgltf::math::fmat4x4>(&source.transform)) {
                //! Options::DecomposeNodeMatrices normally spares us this, but a matrix it refused
                //! to decompose still has move to separate affine transformations
                fastgltf::math::decomposeTransformMatrix(*matrix, scale, rotation, translation);
            }

            outNode->translation = glm::vec3{translation[0], translation[1], translation[2]};
            //! fastgltf stores quaternions xyzw, glm's constructor takes wxyz
            outNode->rotation = glm::quat{rotation[3], rotation[0], rotation[1], rotation[2]};
            outNode->scale = glm::vec3{scale[0], scale[1], scale[2]};
        }

        //! Depth-first flatten of one glTF scene. Emitting each node before descending is what
        //! guarantees a parent's index is smaller than its children's
        void FlattenNode(const fastgltf::Asset& asset, size_t sourceIndex, uint32_t parent,
                         const std::vector<uint32_t>& meshRemap, std::vector<bool>& visited,
                         std::vector<NodeDesc>* outNodes) {
            if (sourceIndex >= asset.nodes.size()) return;
            if (visited[sourceIndex]) {
                LogWarn("GltfLoader: node {} is reachable more than once - the node graph is not a "
                        "tree, so the repeat is dropped", sourceIndex);
                return;
            }
            visited[sourceIndex] = true;

            const fastgltf::Node& source = asset.nodes[sourceIndex];

            NodeDesc node;
            node.name = ToStdString(source.name);
            node.parent = parent;
            node.meshIndex = MODEL_INDEX_NONE;
            if (source.meshIndex.has_value()) {
                const size_t assetMesh = source.meshIndex.value();
                node.meshIndex = assetMesh < meshRemap.size() ? meshRemap[assetMesh] : MODEL_INDEX_NONE;
                if (node.meshIndex == MODEL_INDEX_NONE) {
                    LogWarn("GltfLoader: node '{}' refers to mesh {}, which produced nothing drawable",
                            node.name, assetMesh);
                }
            }
            ConvertNodeTransform(source, &node);

            const auto emittedIndex = static_cast<uint32_t>(outNodes->size());
            outNodes->push_back(std::move(node));

            for (const size_t child : source.children) {
                FlattenNode(asset, child, emittedIndex, meshRemap, visited, outNodes);
            }
        }
    }

    std::optional<ModelData> GltfLoader::LoadFromFile(const std::string& path) {
        const std::filesystem::path filePath{path};

        auto fileData = fastgltf::GltfDataBuffer::FromPath(filePath);
        if (fileData.error() != fastgltf::Error::None) {
            Log(Error, "GltfLoader: could not read '{}': {}", path,
                     fastgltf::getErrorMessage(fileData.error()));
            return std::nullopt;
        }

        //! LoadExternalBuffers so the accessor tools can reach a separate .bin without a custom
        //! adapter; DecomposeNodeMatrices so nodes arrive as TRS; GenerateMeshIndices so a
        //! non-indexed primitive still has an index buffer by the time processing starts
        constexpr auto options = fastgltf::Options::LoadExternalBuffers
                               | fastgltf::Options::DecomposeNodeMatrices
                               | fastgltf::Options::GenerateMeshIndices;

        //! fastgltf's docs say a Parser must not be shared across
        //! threads, and a loader with no state is trivially safe to call from anywhere
        fastgltf::Parser parser;
        auto parsed = parser.loadGltf(fileData.get(), filePath.parent_path(), options);
        if (parsed.error() != fastgltf::Error::None) {
            LogError("GltfLoader: could not parse '{}': {}", path,
                     fastgltf::getErrorMessage(parsed.error()));
            return std::nullopt;
        }
        const fastgltf::Asset& asset = parsed.get();

        ModelData model;
        model.sourcePath = path;

        model.materials.reserve(asset.materials.size());
        for (const fastgltf::Material& material : asset.materials) {
            model.materials.push_back(ConvertMaterial(asset, material));
        }

        uint32_t generatedTangentCount = 0;
        uint32_t fallbackTangentCount = 0;
        uint32_t totalTriangles = 0;

        //! Asset mesh index -> ModelData::meshes index, MODEL_INDEX_NONE where nothing was emitted
        std::vector<uint32_t> meshRemap(asset.meshes.size(), MODEL_INDEX_NONE);

        model.meshes.reserve(asset.meshes.size());
        for (size_t assetMeshIndex = 0; assetMeshIndex < asset.meshes.size(); ++assetMeshIndex) {
            const fastgltf::Mesh& sourceMesh = asset.meshes[assetMeshIndex];
            MeshData mesh;
            mesh.name = ToStdString(sourceMesh.name);

            for (const fastgltf::Primitive& primitive : sourceMesh.primitives) {
                if (primitive.type != fastgltf::PrimitiveType::Triangles) {
                    LogWarn("GltfLoader: mesh '{}' has a non-triangle primitive, skipped", mesh.name);
                    continue;
                }

                VertexStreams streams;
                //! Fill the vertex streams with vertex and index data
                if (!ReadPrimitiveStreams(asset, primitive, &streams)) continue;

                MeshProcessing::ProcessStats stats;
                if (!MeshProcessing::ProcessPrimitive(streams, &stats)) {
                    LogWarn("GltfLoader: a primitive of mesh '{}' failed processing, skipped", mesh.name);
                    continue;
                }
                generatedTangentCount += stats.generatedTangents ? 1 : 0;
                fallbackTangentCount += stats.usedFallbackTangents ? 1 : 0;
                totalTriangles += stats.triangleCount;

                const uint32_t materialIndex = primitive.materialIndex.has_value()
                                                   ? static_cast<uint32_t>(primitive.materialIndex.value())
                                                   : MODEL_INDEX_NONE;
                AppendPrimitive(&mesh, streams, materialIndex);
            }

            if (mesh.submeshes.empty()) {
                LogWarn("GltfLoader: mesh '{}' produced no drawable primitives, skipped", mesh.name);
                continue;
            }

            mesh.bounds = MeshProcessing::ComputeBounds(mesh.streams.positions, 0,
                                                        mesh.streams.GetVertexCount());
            meshRemap[assetMeshIndex] = static_cast<uint32_t>(model.meshes.size());
            model.meshes.push_back(std::move(mesh));
        }

        std::vector<bool> visited(asset.nodes.size(), false);
        const size_t sceneIndex = asset.defaultScene.has_value() ? asset.defaultScene.value() : 0;
        if (sceneIndex < asset.scenes.size()) {
            //! Root node
            for (const size_t rootIndex : asset.scenes[sceneIndex].nodeIndices) {
                FlattenNode(asset, rootIndex, MODEL_INDEX_NONE, meshRemap, visited, &model.nodes);
            }
        } else {
            //! This is when gltf does not have a "scene" but we do not support it for now
            LogWarn("GltfLoader: '{}' defines no scene, so no nodes were emitted", path);
            //! No scene at all is legal glTF. "Every node is a root" would be wrong - a node that
            //! is somebody's child must still be emitted as that child, or it lands in the output
            //! twice with contradictory parents. Only the nodes nobody names are roots
            // std::vector<bool> isChild(asset.nodes.size(), false);
            // for (const fastgltf::Node& node : asset.nodes) {
            //     for (const size_t child : node.children) {
            //         if (child < isChild.size()) isChild[child] = true;
            //     }
            // }
            // for (size_t i = 0; i < asset.nodes.size(); ++i) {
            //     if (isChild[i]) continue;
            //     FlattenNode(asset, i, MODEL_INDEX_NONE, meshRemap, visited, &model.nodes);
            // }
        }

        //! Stat report
        uint32_t vertexCount = 0;
        uint32_t submeshCount = 0;
        for (const MeshData& mesh : model.meshes) {
            vertexCount += mesh.streams.GetVertexCount();
            submeshCount += static_cast<uint32_t>(mesh.submeshes.size());
        }
        LogInfo("GltfLoader: '{}' -> {} meshes, {} submeshes, {} vertices, {} triangles, {} materials, "
                "{} nodes ({} primitives got generated tangents, {} got a fallback basis)",
                path, model.meshes.size(), submeshCount, vertexCount, totalTriangles,
                model.materials.size(), model.nodes.size(), generatedTangentCount, fallbackTangentCount);

        return model;
    }
}
