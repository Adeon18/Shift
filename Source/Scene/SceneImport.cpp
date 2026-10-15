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
            //! Either we failed to load thge texture, or there was no texture and we use a placeholder
            if (!ref.IsSet()) {
                return ref.declared ? textures.GetErrorHandle().slotIdx : textures.GetPlaceholderSlot(ref.placeholderKind);
            }
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
            return scene.AddProperty(node, std::move(property)) != nullptr;
        };

        //! Root model transform
        const NodeID root = scene.CreateNode(std::move(name));
        scene.AddProperty(root, rootTransform);
        scene.AddProperty(root, ModelLockProperty{});

        //! Nodes are already depth dirst sorted, parent -> children -> their children, other parent -> children and so on
        //! So in terms of scene nodes - easy to initialize, as parents always are before children
        std::vector<NodeID> nodes(model->nodes.size(), NodeID{entt::null});
        bool anyMesh = false;
        for (size_t i = 0; i < model->nodes.size(); ++i) {
            const NodeDesc& desc = model->nodes[i];
            const NodeID parent = (desc.parent == MODEL_INDEX_NONE) ? root : nodes[desc.parent];
            nodes[i] = scene.CreateNode(desc.name, parent);
            scene.AddProperty(nodes[i], TransformProperty{.translation = desc.translation, .rotation = desc.rotation, .scale = desc.scale});

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
        const std::string skullPath = root + "Assets/Models/HumanSkull/scene.gltf";

        //! Temp test scene
        constexpr float FLOOR_Y = 0.05f;
        constexpr float SKULL_SCALE = 0.3f;
        constexpr float HELMET_SCALE = 0.7f;
        constexpr float SPHERE_SCALE = 0.5f;
        constexpr uint32_t SKULL_COUNT = 10;

        const auto skullSlot = [&](uint32_t index) {
            const float x = 1.5f + 0.8f * (static_cast<float>(index % 5) - 2.0f);
            const float z = -0.85f + 0.7f * static_cast<float>(index / 5);
            return TransformProperty{.translation = {x, FLOOR_Y + SKULL_SCALE, z}, .scale = glm::vec3{SKULL_SCALE}};
        };

        const std::vector<SceneModel> sources{
            {"DamagedHelmet", root + "Assets/Models/DamagedHelmet/scene.gltf",
             {.translation = {-1.3f, FLOOR_Y + 0.9f * HELMET_SCALE, -0.5f}, .scale = glm::vec3{HELMET_SCALE}}},
            {"HumanSkull", skullPath, skullSlot(0)},
            // No textures, every mat slot is default
            {"Sphere", root + "Assets/Models/Sphere/sphere.glb",
             {.translation = {-3.5f, FLOOR_Y + SPHERE_SCALE, 0.3f}, .scale = glm::vec3{SPHERE_SCALE}}},
            //! TODO: LOCAL, remmove
            {"Sponza", "C:/Users/ade0n/Pictures/3DModels/Sponza/glTF/Sponza.gltf", {}},
        };

        NodeID skull{entt::null};
        for (const SceneModel& source : sources) {
            const NodeID model = ImportModel(scene, ctx, source.path, source.name, source.transform);
            if (source.path == skullPath) { skull = model; }
        }

        for (uint32_t i = 1; i < SKULL_COUNT; ++i) {
            if (TransformProperty* transform = scene.EditProperty<TransformProperty>(scene.CopySubtree(skull))) {
                const TransformProperty slot = skullSlot(i);
                transform->translation = slot.translation;
                transform->scale = slot.scale;
            }
        }

        const auto meshView = scene.GetRegistry().view<const MeshProperty>();
        const size_t meshNodes = meshView.size();
        CheckCritical(meshNodes <= Conf::MAX_SCENE_OBJECTS, "The scene holds more objects than one ObjectData ring slot can carry!");
        size_t submeshInstances = 0;
        for (const auto [node, meshProp] : meshView.each()) {
            //! Counted the way Extract emits them, per submesh of the mesh
            if (const Graphics::Mesh* mesh = ctx.meshes.Get(meshProp.mesh)) { submeshInstances += mesh->submeshes.size(); }
        }
        CheckCritical(submeshInstances <= Conf::MAX_SCENE_SUBMESH_INSTANCES, "The scene holds more submesh instances than one list ring slot can carry!");

        Log(Info, "Scene loaded: {} mesh nodes, {} submesh instances, {} materials, {} vertices and {} indices merged", meshNodes, submeshInstances, ctx.materials.GetCount(), ctx.meshes.GetUsedVertices(), ctx.meshes.GetUsedIndices());

        return true;
    }
}
