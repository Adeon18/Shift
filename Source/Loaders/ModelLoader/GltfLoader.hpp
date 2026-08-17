//
// Created by otrush on 8/15/2026.
//

#ifndef SHIFT_GLTFLOADER_HPP
#define SHIFT_GLTFLOADER_HPP

#include "Loaders/ModelLoader/IModelLoader.hpp"

namespace Shift {

    //! glTF 2.0 / GLB producer for ModelData, on fastgltf.
    class GltfLoader final : public IModelLoader {
    public:
        std::optional<ModelData> LoadFromFile(const std::string& path) override;
    };
}

#endif //SHIFT_GLTFLOADER_HPP
