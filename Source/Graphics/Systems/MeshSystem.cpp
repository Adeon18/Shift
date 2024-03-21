//
// Created by otrush on 3/3/2024.
//

#include "MeshSystem.hpp"
#include "Graphics/UI/UIManager.hpp"

namespace shift::gfx {

    bool IsMeshPassForward(MeshPass pass) {
        switch (pass) {
            case MeshPass::Emission_Forward:
            case MeshPass::Textured_Forward:
                return true;

            return false;
        }
    }

    MeshSystem::MeshSystem(const Device &device, const ShiftBackBuffer &backBufferData, TextureSystem &textureSystem,
                           ModelManager &modelManager, BufferManager &bufferManager, DescriptorManager &descManager,
                           std::unordered_map<ViewSetLayoutType, SGUID>& viewIds): m_device{device}, m_backBufferData{backBufferData},
                           m_textureSystem{textureSystem}, m_modelManager{modelManager}, m_bufferManager{bufferManager}, m_descriptorManager{descManager}, m_perViewIDs{viewIds} {
        CreateDescriptorLayouts();
        CreateRenderStages();
    }

    void MeshSystem::CreateRenderStages() {
        for (auto& [k, v]: RENDER_STAGE_INFOS) {
            switch (v.renderTargetType) {
                case RenderStageCreateInfo::RT_Type::Forward:
                    if (!CreateRenderStageFromInfo(m_device, m_backBufferData, m_descriptorManager, m_renderStagesForward[k], v)) {
                        spdlog::warn("MeshSystem failed to create Mesh Render Stage! Name: {}", v.name);
                    }
                    break;
                case RenderStageCreateInfo::RT_Type::Gbuffer:
                    if (!CreateRenderStageFromInfo(m_device, m_backBufferData, m_descriptorManager, m_renderStagesDeferred[k], v)) {
                        spdlog::warn("MeshSystem failed to create Mesh Render Stage! Name: {}", v.name);
                    }
                    break;
            }
        }
    }

    void MeshSystem::CreateDescriptorLayouts() {
        m_descriptorManager.CreatePerMaterialLayout(
                MaterialSetLayoutType::TEXTURED,
                {
                        {DescriptorType::UBO, 0, VK_SHADER_STAGE_VERTEX_BIT},
                        {DescriptorType::SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
                }
        );

        m_descriptorManager.CreatePerMaterialLayout(
                MaterialSetLayoutType::EMISSION_ONLY,
                {
                        {DescriptorType::UBO, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
                }
        );
    }

    SGUID MeshSystem::AddInstance(MeshPass pass, Mobility mobility, SGUID modelID, const glm::mat4 &transformation, const glm::vec4& color) {
        auto model = m_modelManager.GetModel(modelID);

        for (auto& mesh: model->GetMeshes()) {
            StaticInstance instance{};
            instance.id = GUIDGenerator::GetInstance().Guid();
            instance.modelID = modelID;

            RenderStage& stage = (IsMeshPassForward(pass)) ? m_renderStagesForward[pass]: m_renderStagesDeferred[pass];
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

                instance.data.meshToModel = mesh.meshToModel;
                instance.data.meshToModelInv = mesh.meshToModelInv;
                instance.data.modelToWorld = transformation;
                instance.data.modelToWorldInv = glm::inverse(transformation);
                instance.data.color = color;
                buff.Fill(&instance.data, sizeof(instance.data));
            }

            instance.descriptorSetId = setID;

            switch (mobility) {
                case Mobility::STATIC:
                    m_staticInstances[pass].push_back(instance);
                    return instance.id;
                case Mobility::MOVABLE:
                    m_dynamicInstances[pass][instance.id] = instance;
                    return instance.id;
            }
        }
    }

    void MeshSystem::RenderAllPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame) {
        RenderForwardPasses(buffer, currentImage, currentFrame);
    }

    void MeshSystem::RenderForwardPasses(const CommandBuffer& buffer, uint32_t currentImage, uint32_t currentFrame) {
        // TODO: FOR NOT TO BACKBUFFER
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

        RenderMeshesFromStages(buffer, m_renderStagesForward, currentFrame);

        ui::UIManager::GetInstance().EndFrame(buffer);

        buffer.EndRendering();
    }

    void MeshSystem::UpdateInstances(uint32_t currentFrame) {
        for (const auto &[pass, instances]: m_dynamicInstances) {
            for (auto& [id, ins]: instances) {
                auto& buff = m_bufferManager.GetUBO(ins.descriptorSetId, currentFrame);
                buff.Fill(&ins.data, sizeof(ins.data));
            }
        }
    }

    void MeshSystem::RenderMeshesFromStages(const CommandBuffer& buffer, const std::unordered_map<MeshPass, RenderStage> &renderStages, uint32_t currentFrame) {
        for (auto& [k, v]: renderStages) {
            buffer.BindPipeline(v.pipeline->Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);

            std::array<VkDescriptorSet, 1> sets{ m_descriptorManager.GetPerFrameSet(currentFrame).Get() };
            buffer.BindDescriptorSets(sets, {}, v.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

            std::array<VkDescriptorSet, 1> setsView{ m_descriptorManager.GetPerViewSet(m_perViewIDs[v.viewSetLayoutType], currentFrame).Get() };
            buffer.BindDescriptorSets(setsView, {}, v.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

            for (auto& instance: m_staticInstances[k]) {
                auto model = m_modelManager.GetModel(instance.modelID);
                std::array<VkBuffer, 1> vertexBuffers{ model->GetVertexBufferPtr().Get() };
                std::array<VkDeviceSize, 1> offsets{ 0 };
                buffer.BindVertexBuffers(vertexBuffers, offsets, 0);
                buffer.BindIndexBuffer(model->GetIndexBufferRef().Get(), 0);

                std::array<VkDescriptorSet, 1> setsObj{ m_descriptorManager.GetPerMaterialSet(instance.descriptorSetId, currentFrame).Get() };
                buffer.BindDescriptorSets(setsObj, {}, v.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
                // DRAW THE FUCKING TRIANGLE
                buffer.DrawIndexed(model->GetRanges()[0].indexNum, 1, 0, 0, 0);
            }

            for (auto& [id, instance]: m_dynamicInstances[k]) {
                auto model = m_modelManager.GetModel(instance.modelID);
                std::array<VkBuffer, 1> vertexBuffers{ model->GetVertexBufferPtr().Get() };
                std::array<VkDeviceSize, 1> offsets{ 0 };
                buffer.BindVertexBuffers(vertexBuffers, offsets, 0);
                buffer.BindIndexBuffer(model->GetIndexBufferRef().Get(), 0);

                std::array<VkDescriptorSet, 1> setsObj{ m_descriptorManager.GetPerMaterialSet(instance.descriptorSetId, currentFrame).Get() };
                buffer.BindDescriptorSets(setsObj, {}, v.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
                // DRAW THE FUCKING TRIANGLE
                buffer.DrawIndexed(model->GetRanges()[0].indexNum, 1, 0, 0, 0);
            }
        }
    }

    MeshSystem::StaticInstance &MeshSystem::GetDynamicInstance(MeshPass pass, SGUID id) {
        return m_dynamicInstances[pass][id];
    }

    void MeshSystem::SetDynamicInstanceWorldPosition(MeshPass pass, SGUID id, const glm::vec3 &worldPos) {
        auto &ins = m_dynamicInstances[pass][id];
        if (ins.id == 0) {
            spdlog::warn("MeshSystem: Cannot set instance position because instance at id=" + std::to_string(id) + " does not exist!");
            return;
        }

        ins.data.modelToWorld[3] = glm::vec4(worldPos, 1);
        ins.data.modelToWorldInv = glm::inverse(ins.data.modelToWorld);
    }

    void MeshSystem::SetEmissionPassInstanceColor(SGUID id, const glm::vec4& color) {
        auto &ins = m_dynamicInstances[MeshPass::Emission_Forward][id];
        if (ins.id == 0) {
            spdlog::warn("MeshSystem: Cannot set forward instance color because instance at id=" + std::to_string(id) + " does not exist!");
            return;
        }
        ins.data.color = color;
    }
} // shift::gfx