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

    enum class Mobility {
        STATIC,
        MOVABLE
    };

    class MeshSystem {
    public:
        MeshSystem(const Device& device, const ShiftBackBuffer& backBufferData, TextureSystem& textureSystem, ModelManager& modelManager, BufferManager& bufferManager, DescriptorManager &descManager);

        void AddInstance(MeshPass pass, Mobility mobility, SGUID modelID, const glm::mat4& transformation);

        //! Render all passes to 1 command buffer
        void RenderAllPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame, SGUID perViewID);

        ~MeshSystem() = default;

        MeshSystem() = delete;
        MeshSystem(const MeshSystem&) = delete;
        MeshSystem& operator=(const MeshSystem&) = delete;
    private:
        struct StaticInstance {
            SGUID id;
            SGUID modelID;
            SGUID descriptorSetId;
        };

        void CreateRenderStages();

        const Device& m_device;
        const ShiftBackBuffer& m_backBufferData;
        TextureSystem& m_textureSystem;
        ModelManager& m_modelManager;
        DescriptorManager& m_descriptorManager;
        BufferManager& m_bufferManager;

        std::unordered_map<MeshPass, RenderStage> m_renderStages;
        std::unordered_map<MeshPass, std::vector<StaticInstance>> m_staticInstances;
        // TODO: dynamic instances
        //std::unordered_map<MeshPass, std::vector<StaticInstance>> m_staticInstances;

        std::unordered_map<MeshPass, RenderStageCreateInfo> RENDER_STAGE_INFOS {
                {MeshPass::Emission,
                 {
                     .name = "Emission",
                     .shaderData = {"EmissionForward.vert.spv", "EmissionForward.frag.spv", "", "", ""},
                     .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                     .matSetLayoutType = MaterialSetLayoutType::EMISSION_ONLY,
                     .renderTargetType = RenderStageCreateInfo::RT_Type::Forward
                 }
                },
                {MeshPass::Textured,
                 {
                         .name = "Textured",
                         .shaderData = {"Textured.vert.spv", "Textured.frag.spv", "", "", ""},
                         .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                         .matSetLayoutType = MaterialSetLayoutType::TEXTURED,
                         .renderTargetType = RenderStageCreateInfo::RT_Type::Forward
                 }
                },
        };
    };
} // shift::gfx

#endif //SHIFT_MESHSYSTEM_HPP
