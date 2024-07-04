//
// Created by otrush on 3/3/2024.
//

#ifndef SHIFT_GEOMETRYSYSTEM_HPP
#define SHIFT_GEOMETRYSYSTEM_HPP

#include <unordered_map>

#include "RenderStage.hpp"

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Images/TextureManager.hpp"
#include "Graphics/Abstraction/Geometry/ModelManager.hpp"
#include "Graphics/Abstraction/Images/RenderTargetManager.hpp"
#include "Graphics/Abstraction/Descriptors/BufferManager.hpp"
#include "Graphics/ShiftContextData.hpp"

#include "Graphics/Abstraction/Descriptors/UBOStructs.hpp"
#include "Graphics/Abstraction/Images/SamplerManager.hpp"

namespace shift::gfx {
    enum class MeshPass {
        Emission_Forward,
        Textured_Forward,
        SimpleLights_Forward,
        PBR_Forward
    };

    enum class Mobility {
        STATIC,
        MOVABLE
    };

    [[nodiscard]] bool IsMeshPassForward(MeshPass pass);

    class GeometrySystem {
    private:
        struct GeometryInstance {
            PerDefaultObject data;

            SGUID meshId = 0;
            SGUID instanceID;
            SGUID modelID;
            SGUID descriptorSetId;
        };
    public:
        GeometrySystem(
                const Device& device,
                const ShiftBackBuffer& backBufferData,
                const SamplerManager& samplerManager,
                TextureManager& textureSystem,
                ModelManager& modelManager,
                BufferManager& bufferManager,
                DescriptorManager &descManager,
                RenderTargetManager& RTSystem,
                std::unordered_map<ViewSetLayoutType, SGUID>& viewIds);

        SGUID AddInstance(MeshPass pass, Mobility mobility, SGUID modelID, const glm::mat4& transformation, const glm::vec4& color = {0.0f, 0.0f, 0.0f, 0.0f});

        //! TODO: Replace with separate fucntions for handling movement, etc
        GeometryInstance& GetDynamicInstance(MeshPass pass, SGUID id);

        void SetDynamicInstanceWorldPosition(MeshPass pass, SGUID id, const glm::vec3& worldPosition);
        void SetDynamicInstanceWorldTransform(MeshPass pass, SGUID id, const glm::vec3& worldPosition, const glm::vec3& worldScale = glm::vec3(1.0f), const glm::vec4& worldAxisRotationDeg = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

        //! TODO: Temporary function to set color for lights, used by LightSystem
        void SetEmissionPassInstanceColor(SGUID id, const glm::vec4& color);

        //! Render all passes to 1 command buffer
        void RenderAllPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame);

        void RenderForwardPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame);

        //! Update the UBO data of dynamic instances
        void UpdateInstances(uint32_t currentFrame);

        ~GeometrySystem() = default;

        GeometrySystem() = delete;
        GeometrySystem(const GeometrySystem&) = delete;
        GeometrySystem& operator=(const GeometrySystem&) = delete;
    private:
        void RenderMeshesFromStages(const CommandBuffer& buffer, std::unordered_map<MeshPass, RenderStage> &renderStages, uint32_t currentFrame);

//        struct DynamicInstance {
//            PerDefaultObject data;
//
//            SGUID id;
//            SGUID modelID;
//            SGUID descriptorSetId;
//        };

        //! Create all the render stages
        void CreateRenderStages();

        //! Create the descriptor layouts used by the render stages
        void CreateDescriptorLayouts();

        const Device& m_device;
        const ShiftBackBuffer& m_backBufferData;
        const SamplerManager& m_samplerManager;
        TextureManager& m_textureSystem;
        ModelManager& m_modelManager;
        DescriptorManager& m_descriptorManager;
        BufferManager& m_bufferManager;
        RenderTargetManager& m_RTSystem;
        std::unordered_map<ViewSetLayoutType, SGUID>& m_perViewIDs;

        std::unordered_map<MeshPass, RenderStage> m_renderStagesForward;
        std::unordered_map<MeshPass, RenderStage> m_renderStagesDeferred;
        std::unordered_map<MeshPass, std::vector<GeometryInstance>> m_staticInstances;
        // TODO: dynamic instances
        std::unordered_map<MeshPass, std::unordered_map<SGUID, GeometryInstance>> m_dynamicInstances;

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
                {MeshPass::PBR_Forward,
                        {
                                .name = "PBR",
                                .shaderData = {"PBRCookTorrance.vert.spv", "PBRCookTorrance.frag.spv", "", "", ""},
                                .viewSetLayoutType = ViewSetLayoutType::DEFAULT_CAMERA,
                                .matSetLayoutType = MaterialSetLayoutType::PBR,
                                .renderTargetType = RenderStageCreateInfo::RT_Type::Forward
                        }
                },
        };
    };
} // shift::gfx

#endif //SHIFT_GEOMETRYSYSTEM_HPP
