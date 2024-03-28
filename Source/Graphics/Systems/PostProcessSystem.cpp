//
// Created by otrush on 3/26/2024.
//
#include "PostProcessSystem.hpp"

namespace shift::gfx {
    PostProcessSystem::PostProcessSystem(const Device &device,
                           const ShiftBackBuffer &backBufferData,
                           const SamplerManager& samplerManager,
                           DescriptorManager &descManager,
                           RenderTargetSystem& RTSystem):
            m_device{device},
            m_backBufferData{backBufferData},
            m_samplerManager{samplerManager},
            m_descriptorManager{descManager},
            m_RTSystem{RTSystem}
    {
        CreateDescriptorLayouts();
        CreateRenderStages();


        m_postProcessSetGuid = m_descriptorManager.AllocatePerMaterialSet(MaterialSetLayoutType::POST_PROCESS);
        for (uint32_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            // TODO: should be dependent on material, AND THIS IS SO SHIT
            auto& perObjSet = m_descriptorManager.GetPerMaterialSet(m_postProcessSetGuid, i);
            perObjSet.UpdateImage(0, m_RTSystem.GetColorRT(RenderTargetSystem::HDR_BUFFER).GetView(), m_samplerManager.GetPointSampler());
            perObjSet.ProcessUpdates();
        }
    }

    void PostProcessSystem::CreateRenderStages() {
        for (auto& [k, v]: RENDER_STAGE_INFOS) {
            if (!CreateRenderStageFromInfo(m_device, m_backBufferData, m_descriptorManager, m_RTSystem, m_postProcessStages[k], v)) {
                spdlog::warn("PostProcessSystem failed to create PostProcess Render Stage! Name: {}", v.name);
            }
            spdlog::debug("Created PostProcess render stage: " + v.name);
        }
    }

    void PostProcessSystem::CreateDescriptorLayouts() {
        m_descriptorManager.CreatePerMaterialLayout(
                MaterialSetLayoutType::POST_PROCESS,
                {
                        {DescriptorType::SAMPLER, 0, VK_SHADER_STAGE_FRAGMENT_BIT}
                }
        );
    }

    void PostProcessSystem::ProcessResize() {
        for (uint32_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {

            auto &perObjSet = m_descriptorManager.GetPerMaterialSet(m_postProcessSetGuid, i);
            perObjSet.UpdateImage(0, m_RTSystem.GetColorRT(RenderTargetSystem::HDR_BUFFER).GetView(),
                                  m_samplerManager.GetPointSampler());
            perObjSet.ProcessUpdates();
        }
    }

    void PostProcessSystem::ToneMap(const CommandBuffer &buffer, uint32_t currentImage, uint32_t currentFrame) {

        auto colorAttInfo = info::CreateRenderingAttachmentInfo(m_backBufferData.swapchain->GetImageViews()[currentImage]);
//        auto depthAttInfo = info::CreateRenderingAttachmentInfo(m_backBufferData.swapchain->GetDepthBufferView(), false, {1.0f, 0});

        VkRenderingInfoKHR renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderInfo.renderArea = {.offset = {0, 0}, .extent = m_backBufferData.swapchain->GetExtent()};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttInfo;
//        renderInfo.pDepthAttachment = &depthAttInfo;

        buffer.SetViewPort(m_backBufferData.swapchain->GetViewport());
        buffer.SetScissor(m_backBufferData.swapchain->GetScissor());

        buffer.BeginRendering(renderInfo);

        auto& stage = m_postProcessStages[m_UI.chosenOperator];

        buffer.BindPipeline(stage.pipeline->Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);

        std::array<VkDescriptorSet, 1> sets{ m_descriptorManager.GetPerMaterialSet(m_postProcessSetGuid, currentFrame).Get() };
        buffer.BindDescriptorSets(sets, {}, stage.pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

        // DRAW THE FUCKING TRIANGLE
        buffer.Draw(3, 1, 0, 0);

        ui::UIManager::GetInstance().EndFrame(buffer);

        buffer.EndRendering();
    }

    void PostProcessSystem::UI::Show(uint32_t currentFrame) {
        if (m_shown) {
            ImGui::Begin(m_name.c_str(), &m_shown);

            // From ImGui Demo; line 1281
            if (ImGui::BeginCombo("ToneMap Operator", m_toneMapOperatorNames[static_cast<size_t>(chosenOperator)], 0))
            {
                for (int n = 0; n < m_toneMapOperatorNames.size(); n++)
                {
                    const bool isSelected = (m_toneMapOperatorNames.size() == n);
                    if (ImGui::Selectable(m_toneMapOperatorNames[n], isSelected))
                        chosenOperator = static_cast<ToneMapPass>(n);

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::End();
        }
    }
} // shift::gfx