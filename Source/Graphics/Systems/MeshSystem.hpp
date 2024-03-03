//
// Created by otrush on 3/3/2024.
//

#ifndef SHIFT_MESHSYSTEM_HPP
#define SHIFT_MESHSYSTEM_HPP

#include <unordered_map>

#include "RenderStage.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Systems/TextureSystem.hpp"
#include "Graphics/Systems/ModelManager.hpp"
#include "Graphics/Abstraction/Descriptors/BufferManager.hpp"
#include "Graphics/ShiftContextData.hpp"

namespace shift::gfx {
    enum class MeshPass {
        Emission,
        Textured
    };

    class MeshSystem {
    public:
        MeshSystem(const Device& device);

        ~MeshSystem() = default;

        MeshSystem() = delete;
        MeshSystem(const MeshSystem&) = delete;
        MeshSystem& operator=(const MeshSystem&) = delete;
    private:
        const Device& m_device;
        const ShiftBackBuffer& m_backBufferData;
        TextureSystem& m_textureSystem;
        ModelManager& m_modelManager;
        DescriptorManager& m_descriptorManager;
        BufferManager& m_bufferManager;

        std::unordered_map<MeshPass, RenderStage> m_renderStages;
    };
} // shift::gfx

#endif //SHIFT_MESHSYSTEM_HPP
