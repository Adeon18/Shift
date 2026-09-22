//
// Created by otrush on 9/22/2026.
//

#ifndef SHIFT_SCENEIMPORT_HPP
#define SHIFT_SCENEIMPORT_HPP

#include <string>

#include "Graphics/Managers/MaterialManager.hpp"
#include "Graphics/Managers/MeshManager.hpp"
#include "Graphics/Managers/TextureManager.hpp"
#include "Graphics/RHI/RHI.hpp"

#include "NodeID.hpp"
#include "ShiftScene.hpp"

//! The file that connects graphics manager -> scene
namespace Shift {
    //! The main guys
    struct ImportContext {
        Graphics::MeshManager& meshes;
        Graphics::MaterialManager& materials;
        Graphics::TextureManager& textures;
        RenderContextEncoder& transferEncoder;
    };

    //! Texture ids are relative to the model file, so one resolver per model
    [[nodiscard]] Graphics::TextureSlotResolver MakeTextureResolver(Graphics::TextureManager& textures, const std::string& baseDir, RenderContextEncoder& transferEncoder);

    //! Uploads one model  and pro[agates the scene structure. Returns root or null depending on success/failure
    NodeID ImportModel(ShiftScene& scene, const ImportContext& ctx, const std::string& path, std::string name, const TransformProperty& rootTransform);

    //! TODO: hardcoded boot scene
    bool LoadScene(ShiftScene& scene, const ImportContext& ctx);
}

#endif //SHIFT_SCENEIMPORT_HPP
