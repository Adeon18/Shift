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
        Emission_Forward,
        Textured_Forward,
        SimpleLights_Forward
    };

    enum class Mobility {
        STATIC,
        MOVABLE
    };

    [[nodiscard]] bool IsMeshPassForward(MeshPass pass);

    class MeshSystem {
    public:
        MeshSystem(const Device& device, const ShiftBackBuffer& backBufferData, TextureSystem& textureSystem, ModelManager& modelManager, BufferManager& bufferManager, DescriptorManager &descManager, std::unordered_map<ViewSetLayoutType, SGUID>& viewIds);

        void AddInstance(MeshPass pass, Mobility mobility, SGUID modelID, const glm::mat4& transformation, const glm::vec4& color = {0.0f, 0.0f, 0.0f, 0.0f});

        //! Render all passes to 1 command buffer
        void RenderAllPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame);

        void RenderForwardPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame);

        ~MeshSystem() = default;

        MeshSystem() = delete;
        MeshSystem(const MeshSystem&) = delete;
        MeshSystem& operator=(const MeshSystem&) = delete;
    private:
        void RenderMeshesFromStages(const CommandBuffer& buffer, const std::unordered_map<MeshPass, RenderStage> &renderStages, uint32_t currentFrame);

        struct StaticInstance {
            SGUID id;
            SGUID modelID;
            SGUID descriptorSetId;
        };

        //! Create all the render stages
        void CreateRenderStages();

        //! Create the descriptor layouts used by the render stages
        void CreateDescriptorLayouts();

        const Device& m_device;
        const ShiftBackBuffer& m_backBufferData;
        TextureSystem& m_textureSystem;
        ModelManager& m_modelManager;
        DescriptorManager& m_descriptorManager;
        BufferManager& m_bufferManager;
        std::unordered_map<ViewSetLayoutType, SGUID>& m_perViewIDs;

        std::unordered_map<MeshPass, RenderStage> m_renderStagesForward;
        std::unordered_map<MeshPass, RenderStage> m_renderStagesDeferred;
        std::unordered_map<MeshPass, std::vector<StaticInstance>> m_staticInstances;
        // TODO: dynamic instances
        //std::unordered_map<MeshPass, std::vector<StaticInstance>> m_staticInstances;

        std::unordered_map<MeshPass, RenderStageCreateInfo> RENDER_STAGE_INFOS {
                {MeshPass::Emission_Forward,
                 {
                     .name = "Emission",
                     .shaderData = {"EmissionForward.vert.spv", "EmissionForward.frag.spv", "", "", ""},
                     .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                     .matSetLayoutType = MaterialSetLayoutType::EMISSION_ONLY,
                     .renderTargetType = RenderStageCreateInfo::RT_Type::Forward
                 }
                },
                {MeshPass::Textured_Forward,
                 {
                         .name = "Textured",
                         .shaderData = {"Textured.vert.spv", "Textured.frag.spv", "", "", ""},
                         .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                         .matSetLayoutType = MaterialSetLayoutType::TEXTURED,
                         .renderTargetType = RenderStageCreateInfo::RT_Type::Forward
                 }
                },
                {MeshPass::SimpleLights_Forward,
                        {
                                .name = "SimpleLights",
                                .shaderData = {"SimpleLights.vert.spv", "SimpleLights.frag.spv", "", "", ""},
                                .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                                .matSetLayoutType = MaterialSetLayoutType::TEXTURED,
                                .renderTargetType = RenderStageCreateInfo::RT_Type::Forward
                        }
                },
        };
    };
} // shift::gfx

#endif //SHIFT_MESHSYSTEM_HPP
