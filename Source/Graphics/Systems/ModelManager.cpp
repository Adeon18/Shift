#include <iostream>
#include "ModelManager.hpp"

#include "Utility/UtilStandard.hpp"
#include "Utility/Vulkan/InfoUtil.hpp"

#include <glm/gtx/string_cast.hpp>

namespace shift::gfx {
    std::shared_ptr<Model> ModelManager::GetModel(SGUID id)
    {
        if (m_loadedModels.find(id) != m_loadedModels.end()) {
            return m_loadedModels[id];
        }

        spdlog::info("ModelManager: Model is not cached, will perform load.");
    }


    SGUID ModelManager::LoadModel(const std::string& filename)
    {
        Assimp::Importer importer;
        const aiScene* assimpScene = importer.ReadFile(filename, IMPORT_FLAGS);
        if (!assimpScene) {
            spdlog::warn("ModelManager::Failed loading model. Location: " + filename);
            return 0;
        }

        uint32_t numMeshes = assimpScene->mNumMeshes;

        // Load vertex data
        std::shared_ptr<Model> modelPtr = std::shared_ptr<Model>(new Model{});
        //modelPtr->name = filename;
        modelPtr->GetMeshes().resize(numMeshes);

        // Load all meshes t
        for (uint32_t i = 0; i < numMeshes; ++i) {
            auto& assimpMesh = assimpScene->mMeshes[i];
            auto& modelMesh = modelPtr->GetMeshes()[i];

            //modelMesh.name = assimpMesh->mName.C_Str();

            auto maxVec = assimpMesh->mAABB.mMax;
            auto minVec = assimpMesh->mAABB.mMin;

            modelMesh.vertices.resize(assimpMesh->mNumVertices);
            modelMesh.triangles.resize(assimpMesh->mNumFaces);

            // Triangle vertices
            for (uint32_t v = 0; v < assimpMesh->mNumVertices; ++v)
            {
                Vertex& vertex = modelMesh.vertices[v];
                vertex.pos = util::ass::ToGlm(assimpMesh->mVertices[v]);
                //! WARNING! IS THIS RIGHT???? WE WILL SEEEE
                auto tcVec3 = util::ass::ToGlm(assimpMesh->mTextureCoords[0][v]);
                vertex.texCoord = glm::vec2{ tcVec3.x,  1.0f - tcVec3.y };
                vertex.normal = util::ass::ToGlm(assimpMesh->mNormals[v]);
                vertex.tangent = util::ass::ToGlm(assimpMesh->mTangents[v]);

                // BIG WARNING: !!!
                vertex.bitangent = util::ass::ToGlm(assimpMesh->mBitangents[v]);
                //vertex.bitangent = XMFLOAT3{ bitanOrig.x * -1.f, bitanOrig.y * -1.f, bitanOrig.z * -1.f }; // Flip V
            }
            // Triangle faces
            for (uint32_t f = 0; f < assimpMesh->mNumFaces; ++f)
            {
                const auto& face = assimpMesh->mFaces[f];
                for (uint32_t j = 0; j < face.mNumIndices; ++j) {
                    modelMesh.triangles[f].indices[j] = face.mIndices[j];
                }
            }

        }

        // Load the textures
        PrintAllTexturesPath(assimpScene);
        LoadTextures(assimpScene, modelPtr, aiTextureType_DIFFUSE, filename);
//        LoadTextures(assimpScene, modelPtr, aiTextureType_NORMALS, filename);
//        LoadTextures(assimpScene, modelPtr, aiTextureType_SHININESS, filename);
//        LoadTextures(assimpScene, modelPtr, aiTextureType_METALNESS, filename);

        std::function<void(aiNode*)> loadInstances = [&loadInstances, &modelPtr](aiNode* node)
        {
            const glm::mat4 nodeToParent = util::ass::ToGlm(node->mTransformation.Transpose());

            const glm::mat4 parentToNode = glm::inverse(nodeToParent);

            // TODO: TEMPORARY FIX
            // The same node may contain multiple meshes in its space, referring to them by indices
//            for (uint32_t i = 0; i < node->mNumMeshes; ++i)
//            {
//                uint32_t meshIndex = node->mMeshes[i];
                modelPtr->GetMeshes()[0].meshToModel = nodeToParent;
                modelPtr->GetMeshes()[0].meshToModelInv = parentToNode;
//            }

//            for (uint32_t i = 0; i < node->mNumChildren; ++i) {
//                loadInstances(node->mChildren[i]);
//            }
        };

        loadInstances(assimpScene->mRootNode);

        modelPtr->InitWithMeshData(m_device, m_pool.RequestCommandBuffer());

        SGUID modelID = GUIDGenerator::GetInstance().Guid();

        m_loadedModels[modelID] = modelPtr;
        return modelID;
    }
    void ModelManager::LoadTextures(const aiScene* pScene, std::shared_ptr<Model> modelPtr, aiTextureType textureType, const std::string& filename)
    {
        uint32_t numMeshes = pScene->mNumMeshes;

        // Load all meshes textures
        for (uint32_t i = 0; i < numMeshes; ++i) {
            auto& assimpMesh = pScene->mMeshes[i];
            auto& modelMesh = modelPtr->GetMeshes()[i];
            // Load textures
            aiMaterial* material = pScene->mMaterials[assimpMesh->mMaterialIndex];
            uint32_t textureCount = material->GetTextureCount(textureType);

            for (uint32_t t = 0; t < textureCount; ++t) {
                aiString path;
                material->GetTexture(textureType, t, &path);
                std::string fullTexturePath = util::GetDirectoryFromPath(filename) + path.C_Str();
                // Load texture by full path and save the full path
                auto guid = m_textureSystem.LoadTexture(fullTexturePath, VK_FORMAT_R8G8B8A8_SRGB);
                modelMesh.texturePaths[MeshTextureType::DIFFUSE] = guid;
                m_textureSystem.GetTexture(guid)->CreateSampler(info::CreateSamplerInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
            }
        }
    }

    void ModelManager::PrintAllTexturesPath(const aiScene* pScene)
    {
        uint32_t numMeshes = pScene->mNumMeshes;
        std::cout << numMeshes << std::endl;
        for (uint32_t i = 0; i < numMeshes; ++i) {
            auto& pMesh = pScene->mMeshes[i];
            aiMaterial* material = pScene->mMaterials[pMesh->mMaterialIndex];
            for (auto texType : TEXTURE_TYPES) {
                uint32_t textureCount = material->GetTextureCount(texType);
                if (textureCount == 0) continue;
                std::cout << "Type: " << aiTextureTypeToString(texType) << " : Count : " << textureCount << std::endl;
                for (uint32_t t = 0; t < textureCount; ++t) {
                    aiString path;
                    material->GetTexture(texType, t, &path);
                    std::cout << "path: " << path.C_Str() << std::endl;
                }
            }
        }
    }
}
