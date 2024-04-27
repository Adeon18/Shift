#ifndef SHIFT_MODELSYSTEM_HPP
#define SHIFT_MODELSYSTEM_HPP

#include <unordered_map>

#include "assimp/Importer.hpp"      // C++ importer interface
#include "assimp/scene.h"           // Output data structure
#include "assimp/postprocess.h"     // Post processing flags

#include "Model.hpp"
#include "Graphics/Abstraction/Images/TextureSystem.hpp"

namespace shift::gfx {
    class ModelSystem {
    public:
        static constexpr uint32_t IMPORT_FLAGS = uint32_t(
                aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices  | aiProcess_PreTransformVertices
        );
    public:
        ModelSystem(const Device& device, CommandPool& pool, TextureSystem& texSys): m_device{device}, m_pool{pool}, m_textureSystem{texSys} {}

        ModelSystem() = default;
        ModelSystem(const ModelSystem& other) = delete;
        ModelSystem& operator=(const ModelSystem& other) = delete;

        //! Get model By Id, return null if not present, maybe TODO: slow
        std::shared_ptr<Model> GetModel(SGUID id);

        //! Load and cache the model into a map
        SGUID LoadModel(const std::string& filename);
    private:

        void LoadTextures(const aiScene* pScene, std::shared_ptr<Model> modelPtr, aiTextureType srcTexType, MeshTextureType dstTexType, const std::string& filename, bool generateMips = false);
        //! Print all the textures paths for each mesh given the scene
        void PrintAllTexturesPath(const aiScene* pScene);
    private:
        const Device& m_device;
        CommandPool& m_pool;
        TextureSystem& m_textureSystem;

        std::unordered_map<SGUID, std::shared_ptr<Model>> m_loadedModels;

        const std::vector<aiTextureType> TEXTURE_TYPES{
                aiTextureType_NONE,
                aiTextureType_DIFFUSE,
                aiTextureType_SPECULAR,
                aiTextureType_AMBIENT,
                aiTextureType_EMISSIVE,
                aiTextureType_HEIGHT,
                aiTextureType_NORMALS,
                aiTextureType_SHININESS,
                aiTextureType_OPACITY,
                aiTextureType_DISPLACEMENT,
                aiTextureType_LIGHTMAP,
                aiTextureType_REFLECTION,
                aiTextureType_BASE_COLOR,
                aiTextureType_NORMAL_CAMERA,
                aiTextureType_EMISSION_COLOR,
                aiTextureType_METALNESS,
                aiTextureType_DIFFUSE_ROUGHNESS,
                aiTextureType_AMBIENT_OCCLUSION,
                aiTextureType_UNKNOWN
        };
    };
} // shift::gfx

#endif //SHIFT_MODELSYSTEM_HPP
