//
// Created by otrush on 3/3/2024.
//

#include "MeshSystem.hpp"

#include "Graphics/Abstraction/Descriptors/UBOStructs.hpp"

namespace shift::gfx {

    MeshSystem::MeshSystem(const Device &device, const ShiftBackBuffer &backBufferData, TextureSystem &textureSystem,
                           ModelManager &modelManager, BufferManager &bufferManager, DescriptorManager &descManager): m_device{device}, m_backBufferData{backBufferData},
                           m_textureSystem{textureSystem}, m_modelManager{modelManager}, m_bufferManager{bufferManager}, m_descriptorManager{descManager} {
        CreateRenderStages();
    }

    void MeshSystem::CreateRenderStages() {
        for (auto& [k, v]: RENDER_STAGE_INFOS) {
            if (!CreateRenderStageFromInfo(m_device, m_backBufferData, m_descriptorManager, m_renderStages[k], v)) {
                spdlog::warn("MeshSystem failed to create Mesh Render Stage! Name: {}", v.name);
            }
        }
    }

    void MeshSystem::AddInstance(MeshPass pass, Mobility mobility, SGUID modelID, const glm::mat4 &transformation) {
        auto model = m_modelManager.GetModel(modelID);

        for (auto& mesh: model->GetMeshes()) {
            StaticInstance instance{};
            instance.id = GUIDGenerator::GetInstance().Guid();
            instance.modelID = modelID;

            auto& stage = m_renderStages[pass];
            SGUID setID = m_descriptorManager.AllocatePerMaterialSet(stage.matSetLayoutType);
            m_bufferManager.AllocateUBO(setID, sizeof(PerDefaultObject));

            auto* tex = m_textureSystem.GetTexture(mesh.texturePaths[gfx::MeshTextureType::DIFFUSE]);

            for (uint32_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
                // TODO: should be dependent on material, AND THIS IS SO SHIT
                auto& perObjSet = m_descriptorManager.GetPerMaterialSet(setID, i);
                auto& buff = m_bufferManager.GetUBO(setID, i);
                switch (stage.matSetLayoutType) {
                    case MaterialSetLayoutType::TEXTURED:
                        perObjSet.UpdateUBO(0, buff.Get(), 0, buff.GetSize());
                        perObjSet.UpdateImage(1, tex->GetView(), tex->GetSampler());
                        break;
                    case MaterialSetLayoutType::EMISSION_ONLY:
                        perObjSet.UpdateUBO(0, buff.Get(), 0, buff.GetSize());
                        break;
                }
                perObjSet.ProcessUpdates();

                PerDefaultObject po{};
                po.meshToModel = mesh.meshToModel;
                po.meshToModelInv = mesh.meshToModelInv;
                po.modelToWorld = transformation;
                po.modelToWorldInv = glm::inverse(transformation);
                buff.Fill(&po, sizeof(po));
            }

            instance.descriptorSetId = setID;

            m_staticInstances[pass].push_back(instance);
        }
    }

    void MeshSystem::RenderAllPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame, SGUID perViewID) {
        // TODO: THIS SHOULD BE DEPENDENT ON THE RenderStage AND ON SWITCHING BEGIN RENDER INFO, MY FUCKING ASS
        auto colorAttInfo = info::CreateRenderingAttachmentInfo(m_backBufferData.swapchain->GetImageViews()[currentImage]);
        auto depthAttInfo = info::CreateRenderingAttachmentInfo(m_backBufferData.swapchain->GetDepthBufferView(), false, {1.0f, 0});

        VkRenderingInfoKHR renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderInfo.renderArea = {.offset = {0, 0}, .extent = m_backBufferData.swapchain->GetExtent()};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttInfo;
        renderInfo.pDepthAttachment = &depthAttInfo;

        buffer.SetViewPort(m_backBufferData.swapchain->GetViewport());
        buffer.SetScissor(m_backBufferData.swapchain->GetScissor());

        buffer.BeginRendering(renderInfo);

        for (auto& [k, v]: m_renderStages) {
            buffer.BindPipeline(v.pipeline->Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);

            for (auto& instance: m_staticInstances[k]) {
                auto model = m_modelManager.GetModel(instance.modelID);
                std::array<VkBuffer, 1> vertexBuffers{ model->GetVertexBufferPtr().Get() };
                std::array<VkDeviceSize, 1> offsets{ 0 };
                buffer.BindVertexBuffers(vertexBuffers, offsets, 0);
                buffer.BindIndexBuffer(model->GetIndexBufferRef().Get(), 0);

                // Bind the descriptor sets
                // INFO: The sets are not unique to pipelines, so we need to specify whether to bind it to compute pipeline or the graphics one
                std::array<VkDescriptorSet, 1> sets{ m_descriptorManager.GetPerFrameSet(currentFrame).Get() };
                buffer.BindDescriptorSets(sets, {}, v.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

                std::array<VkDescriptorSet, 1> setsView{ m_descriptorManager.GetPerViewSet(perViewID, currentFrame).Get() };
                buffer.BindDescriptorSets(setsView, {}, v.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

                std::array<VkDescriptorSet, 1> setsObj{ m_descriptorManager.GetPerMaterialSet(instance.descriptorSetId, currentFrame).Get() };
                buffer.BindDescriptorSets(setsObj, {}, v.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
                // DRAW THE FUCKING TRIANGLE
                buffer.DrawIndexed(model->GetRanges()[0].indexNum, 1, 0, 0, 0);
            }
        }
        buffer.EndRendering();
    }
} // shift::gfx