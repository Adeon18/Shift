//
// Created by otrush on 9/22/2026.
//

#include "SceneImport.hpp"

#include <optional>
#include <utility>
#include <vector>

#include "Config/EngineConfig.hpp"
#include "Loaders/ModelLoader/GltfLoader.hpp"
#include "Utility/Assertions.hpp"
#include "Utility/Logging/LogMacros.hpp"
#include "Utility/UtilStandard.hpp"

namespace Shift {
    Graphics::TextureSlotResolver MakeTextureResolver(Graphics::TextureManager& textures, const std::string& baseDir, RenderContextEncoder& transferEncoder) {
        return [&textures, baseDir, encoder = &transferEncoder](const TextureRef& ref) {
            if (!ref.IsSet()) { return textures.GetPlaceholderSlot(ref.placeholderKind); }
            const Graphics::TextureHandle tex = textures.GetOrLoadTexture(baseDir + ref.id, encoder, ref.colorSpace);
            return tex.slotIdx;
        };
    }

    NodeID ImportModel(ShiftScene& scene, const ImportContext& ctx, const std::string& path, std::string name, const TransformProperty& rootTransform) {
        GltfLoader loader;
        std::optional<ModelData> model = loader.LoadFromFile(path);
        if (!model) {
            Log(Warning, "Scene model not loaded, skipping it: {}", path);
            return entt::null;
        }

        //! Get the global remap and make ressolver to index
        const std::vector<uint32_t> materialRemap = ctx.materials.RegisterModelMaterials(
            model->materials, MakeTextureResolver(ctx.textures, Util::GetDirectoryFromPath(model->sourcePath), ctx.transferEncoder));

        //! Upload every mesh once
        std::vector<Graphics::MeshHandle> meshHandles;
        meshHandles.reserve(model->meshes.size());
        for (const MeshData& meshData : model->meshes) {
            meshHandles.push_back(ctx.meshes.UploadMesh(meshData, ctx.transferEncoder, materialRemap));
        }

        //! Materials default to the ones the upload resolved per submesh
        auto addMesh = [&](NodeID node, Graphics::MeshHandle handle) {
            const Graphics::Mesh* mesh = ctx.meshes.Get(handle);
            if (!mesh) { return false; }
            MeshProperty property{.mesh = handle};
            property.materials.reserve(mesh->submeshes.size());
            for (const Graphics::MeshSubmesh& submesh : mesh->submeshes) {
                property.materials.push_back(submesh.materialIndex);
            }
            return scene.AddMesh(node, std::move(property)) != nullptr;
        };

        //! Root model transform
        const NodeID root = scene.CreateNode(std::move(name));
        scene.AddTransform(root, rootTransform);

        //! Nodes are already depth dirst sorted, parent -> children -> their children, other parent -> children and so on
        //! So in terms of scene nodes - easy to initialize, as parents always are before children
        std::vector<NodeID> nodes(model->nodes.size(), NodeID{entt::null});
        bool anyMesh = false;
        for (size_t i = 0; i < model->nodes.size(); ++i) {
            const NodeDesc& desc = model->nodes[i];
            const NodeID parent = (desc.parent == MODEL_INDEX_NONE) ? root : nodes[desc.parent];
            nodes[i] = scene.CreateNode(desc.name, parent);
            scene.AddTransform(nodes[i], {.translation = desc.translation, .rotation = desc.rotation, .scale = desc.scale});

            if (desc.meshIndex == MODEL_INDEX_NONE || desc.meshIndex >= meshHandles.size()) { continue; }
            anyMesh = addMesh(nodes[i], meshHandles[desc.meshIndex]) || anyMesh;
        }

        //! Fallback if no node tree
        if (!anyMesh) {
            Log(Warning, "Model '{}' has no nodes referencing its {} meshes; placing each at the model transform", path, model->meshes.size());
            for (const Graphics::MeshHandle handle : meshHandles) {
                const Graphics::Mesh* mesh = ctx.meshes.Get(handle);
                if (!mesh) { continue; }
                addMesh(scene.CreateNode(mesh->name, root), handle);
            }
        }
        return root;
    }

    bool LoadScene(ShiftScene& scene, const ImportContext& ctx) {
        struct SceneModel {
            std::string name;
            std::string path;
            TransformProperty transform;
        };

        const std::string root = Util::GetShiftRoot();
        const std::vector<SceneModel> sources{
            {"DamagedHelmet", root + "Assets/Models/DamagedHelmet/scene.gltf", {.translation = {-2.0f, 0.0f, -3.5f}}},
            {"HumanSkull", root + "Assets/Models/HumanSkull/scene.gltf", {.translation = {2.0f, 0.0f, -3.5f}}},
            // No textures, every mat slot is default
            {"Sphere", root + "Assets/Models/Sphere/sphere.glb", {.translation = {0.0f, -1.0f, -2.0f}}},
        };

        for (const SceneModel& source : sources) {
            ImportModel(scene, ctx, source.path, source.name, source.transform);
        }

        const size_t meshNodes = scene.GetRegistry().view<const MeshProperty>().size();
        CheckCritical(meshNodes <= Conf::MAX_SCENE_OBJECTS, "The scene holds more objects than one ObjectData ring slot can carry!");

        Log(Info, "Scene loaded: {} mesh nodes, {} materials, {} vertices and {} indices merged", meshNodes, ctx.materials.GetCount(), ctx.meshes.GetUsedVertices(), ctx.meshes.GetUsedIndices());

        return true;
    }
}
