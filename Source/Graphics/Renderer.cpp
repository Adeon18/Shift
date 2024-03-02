//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Graphics/Abstraction/Descriptors/UBOStructs.hpp"
#include "Utility/Vulkan/InfoUtil.hpp"

namespace shift::gfx {
    bool Renderer::Init() {
        try {
            m_context.instance = std::make_unique<shift::gfx::Instance>("TestApp", VK_MAKE_VERSION(1, 0, 0), "ShiftEngine",
                                                                        VK_MAKE_VERSION(1, 0, 0));
            m_backBuffer.windowSurface = std::make_unique<WindowSurface>(m_context.instance->Get(), m_window.GetHandle());
            m_context.device = std::make_unique<shift::gfx::Device>(*m_context.instance, m_backBuffer.windowSurface->Get());
        } catch (VulkanCreateResourceException &e) {
            spdlog::error(e.what());
            return false;
        }

        m_backBuffer.swapchain = std::make_unique<shift::gfx::Swapchain>(*m_context.device, *m_backBuffer.windowSurface, m_window.GetWidth(), m_window.GetHeight());
        if (!m_backBuffer.swapchain->IsValid()) { return false;}

        m_context.graphicsPool = std::make_unique<CommandPool>(*m_context.device, *m_context.instance, POOL_TYPE::GRAPHICS);
        m_context.transferPool = std::make_unique<CommandPool>(*m_context.device, *m_context.instance, POOL_TYPE::TRANSFER);

        m_descriptorManager = std::make_unique<shift::gfx::DescriptorManager>(*m_context.device);
        if (!m_descriptorManager->AllocatePools()) {return false;}
        m_textureSystem = std::make_unique<shift::gfx::TextureSystem>(*m_context.device, *m_context.graphicsPool, *m_context.transferPool);
        m_modelManager = std::make_unique<shift::gfx::ModelManager>(*m_context.device, *m_context.transferPool, *m_textureSystem);
        m_amogus = m_modelManager->GetModel(shift::util::GetShiftRoot() + "Assets/Models/SimpleAmogusPink/scene.gltf");

        TempCreate();
    }

    bool Renderer::RenderFrame(const shift::gfx::EngineData &engineData) {
        auto& buff = m_context.graphicsPool->RequestCommandBuffer(shift::gfx::BUFFER_TYPE::FLIGHT, m_currentFrame);

        //////////////// FUCKING AROUND

        static int fuck = 0;
        fuck++;
        static int index = 0;
        static int framesUpdated = 0;
        bool update;
        if (fuck == 600) {
            framesUpdated = 0;
            index = 1;
            update = true;
        } else if (fuck == 1200) {
            index = 0;
            fuck = 0;
            update = true;
            framesUpdated = 0;
        }
        if (framesUpdated < 2) {
//            auto &perFrameSet = m_descriptorManager->GetPerFrameSet(m_currentFrame);
//
//            auto* tex = m_textureSystem->GetTexture(m_amogus->GetMeshes()[0].texturePaths[gfx::MeshTextureType::DIFFUSE]);
//
//            //perFrameSet.UpdateUBO<PerFrameLegacy>(0, m_uniformBuffers[i], 0);
//            perFrameSet.UpdateImage(1, tex->GetView(), tex->GetSampler());
//            perFrameSet.ProcessUpdates();
//            framesUpdated++;
        }
        ////////////////

        /// Aquire availible swapchain image index
        bool changed = false;
        uint32_t imageIndex = m_backBuffer.swapchain->AquireNextImageIndex(*m_imageAvailableSemaphores[m_currentFrame], &changed);
        if (imageIndex == UINT32_MAX) return false;
        if (changed) {
            if (!m_backBuffer.swapchain->Recreate(m_window.GetWidth(), m_window.GetHeight())) return false;
            return true;
        }

        TempRecordCommandBuffer(buff, imageIndex);

        /// BUffer updates

        PerDefaultView pf{};
        pf.view = engineData.viewMatrix;
        pf.proj = engineData.projMatrix;
        pf.viewInv = glm::inverse(engineData.viewMatrix);
        pf.projInv = glm::inverse(engineData.projMatrix);
        m_uniformBuffers[m_currentFrame]->Fill(&pf, sizeof(pf));

        PerFrame pfr{};
        pfr.camDirection = glm::vec4{engineData.camDirection, 0};
        pfr.camPosition = glm::vec4{engineData.camPosition, 0};
        pfr.camUp = glm::vec4{engineData.camUp, 0};
        pfr.camRight = glm::vec4{engineData.camRight, 0};
        pfr.windowData = glm::vec4{static_cast<float>(engineData.winWidth), static_cast<float>(engineData.winHeight), engineData.oneDivWinWidth, engineData.oneDivWinHeight};
        pfr.timerData = glm::vec4{engineData.dt, engineData.fps, engineData.secondsSinceStart, 0};
        m_uniformBuffersPF[m_currentFrame]->Fill(&pfr, sizeof(pfr));

        PerDefaultObject po{};
        po.meshToModel = m_amogus->GetMeshes()[0].meshToModel;
        po.meshToModelInv = m_amogus->GetMeshes()[0].meshToModelInv;
        po.modelToWorld = glm::mat4(1);
        po.modelToWorldInv = glm::mat4(1);
        m_uniformBuffersPO[m_currentFrame]->Fill(&po, sizeof(po));

        PerDefaultObject po2{};
        po2.meshToModel = m_amogus->GetMeshes()[0].meshToModel;
        po2.meshToModelInv = m_amogus->GetMeshes()[0].meshToModelInv;
        po2.modelToWorld = glm::translate(glm::mat4(1), glm::vec3{10., 0., 0.});
        po2.modelToWorldInv = glm::inverse(po2.modelToWorld);
        m_uniformBuffersPO2[m_currentFrame]->Fill(&po2, sizeof(po2));
        ///

        std::array<VkSemaphore, 1> waitSem{ m_imageAvailableSemaphores[m_currentFrame]->Get() };
        std::array<VkSemaphore, 1> sigSem{ m_renderFinishedSemaphores[m_currentFrame]->Get() };
        std::array<VkCommandBuffer, 1> cmdBuf{ buff.Get() };
        std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        buff.Submit(info::CreateSubmitInfo(
                waitSem,
                sigSem,
                cmdBuf,
                waitStages.data()
        ));

        bool isOld = false;
        bool success = m_backBuffer.swapchain->Present(*m_renderFinishedSemaphores[m_currentFrame], imageIndex, &isOld);
        if (!success) {return false;}

        if (isOld || m_window.ShouldProcessResize()) {
            m_window.ProcessResize();
            m_controller.UpdateScreenSize(m_window.GetWidth(), m_window.GetHeight());
            if (!m_backBuffer.swapchain->Recreate(m_window.GetWidth(), m_window.GetHeight())) return false;
        }

        // Update the current frame
        m_currentFrame = (m_currentFrame + 1) % shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT;

        return true;
    }

    void Renderer::Cleanup() {
        //! Wait for device to finish operations so we can clean everything properly
        vkDeviceWaitIdle(m_context.device->Get());

        m_backBuffer.swapchain.reset();

        m_textureSystem.reset();
        m_amogus.reset();
        m_modelManager.reset();

        // TODO: REMOVE
        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffers[i].reset();
            m_uniformBuffersPF[i].reset();
            m_uniformBuffersPO[i].reset();
            m_uniformBuffersPO2[i].reset();
        }
        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i].reset();
            m_renderFinishedSemaphores[i].reset();
        }
        m_pipeline.reset();
        // End TODO

        m_descriptorManager.reset();

        m_context.graphicsPool.reset();
        m_context.transferPool.reset();

        m_backBuffer.windowSurface.reset();
        m_context.device.reset();
    }

    void Renderer::CreatePipelines() {

    }

    void Renderer::CreateUniformDescriptors() {

    }

    void Renderer::TempCreate() {
        VkDeviceSize bufferSize = sizeof(PerDefaultView);
        m_uniformBuffers.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffers[i] = std::make_unique<shift::gfx::UniformBuffer>(*m_context.device, bufferSize);
        }

        bufferSize = sizeof(PerFrame);
        m_uniformBuffersPF.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffersPF[i] = std::make_unique<shift::gfx::UniformBuffer>(*m_context.device, bufferSize);
        }

        bufferSize = sizeof(PerDefaultObject);
        m_uniformBuffersPO.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffersPO[i] = std::make_unique<shift::gfx::UniformBuffer>(*m_context.device, bufferSize);
        }

        bufferSize = sizeof(PerDefaultObject);
        m_uniformBuffersPO2.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffersPO2[i] = std::make_unique<shift::gfx::UniformBuffer>(*m_context.device, bufferSize);
        }

        for (uint32_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_descriptorManager->CreatePerFrameLayout({{DescriptorType::UBO, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT}});
            m_descriptorManager->CreatePerViewLayout(
                    ViewSetLayoutType::DEFAULT_CAMERA,
                    {
                        {DescriptorType::UBO, 0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
                    }
                    );

            m_descriptorManager->CreatePerMaterialLayout(
                    MaterialSetLayoutType::TEXTURED,
                    {
                            {DescriptorType::UBO, 0, VK_SHADER_STAGE_VERTEX_BIT},
                            {DescriptorType::SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
                    }
            );
        }
        m_perViewID = m_descriptorManager->AllocatePerViewSet(ViewSetLayoutType::DEFAULT_CAMERA);
        m_perFrameID = m_descriptorManager->AllocatePerFrameSet();
        m_perMatID = m_descriptorManager->AllocatePerMaterialSet(MaterialSetLayoutType::TEXTURED);
        m_perMatID2 = m_descriptorManager->AllocatePerMaterialSet(MaterialSetLayoutType::TEXTURED);

        for (uint32_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            auto& perFrameSet = m_descriptorManager->GetPerFrameSet(i);
            perFrameSet.UpdateUBO<PerFrame>(0, m_uniformBuffersPF[i]->Get(), 0);
            perFrameSet.ProcessUpdates();

            auto& perViewSet = m_descriptorManager->GetPerViewSet(m_perViewID, i);
            perViewSet.UpdateUBO<PerDefaultView>(0, m_uniformBuffers[i]->Get(), 0);
            perViewSet.ProcessUpdates();

            auto& perMatSet = m_descriptorManager->GetPerMaterialSet(m_perMatID, i);
            auto* tex = m_textureSystem->GetTexture(m_amogus->GetMeshes()[0].texturePaths[gfx::MeshTextureType::DIFFUSE]);
            perMatSet.UpdateUBO<PerDefaultObject>(0, m_uniformBuffersPO[i]->Get(), 0);
            perMatSet.UpdateImage(1, tex->GetView(), tex->GetSampler());
            perMatSet.ProcessUpdates();

            auto& perMatSet2 = m_descriptorManager->GetPerMaterialSet(m_perMatID2, i);
            perMatSet2.UpdateUBO<PerDefaultObject>(0, m_uniformBuffersPO2[i]->Get(), 0);
            perMatSet2.UpdateImage(1, tex->GetView(), tex->GetSampler());
            perMatSet2.ProcessUpdates();
        }

        m_imageAvailableSemaphores.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i] = std::make_unique<shift::gfx::Semaphore>(*m_context.device);
            m_renderFinishedSemaphores[i] = std::make_unique<shift::gfx::Semaphore>(*m_context.device);
        }



        shift::gfx::Shader vert{*m_context.device, shift::util::GetShiftRoot() + "Shaders/shader.vert.spv", shift::gfx::Shader::Type::Vertex};
        if (!vert.CreateStage()) {}

        shift::gfx::Shader frag{*m_context.device, shift::util::GetShiftRoot() + "Shaders/shader.frag.spv", shift::gfx::Shader::Type::Fragment};
        if (!frag.CreateStage()) {}

        m_pipeline = std::make_unique<shift::gfx::Pipeline>(*m_context.device);

        m_pipeline->AddShaderStage(vert);
        m_pipeline->AddShaderStage(frag);

        auto bindingDescription = gfx::Vertex::getBindingDescription();
        auto attributeDescriptions = gfx::Vertex::getAttributeDescriptions();
        m_pipeline->SetInputStateInfo(shift::info::CreateInputStateInfo(attributeDescriptions, {&bindingDescription, 1}), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        m_pipeline->SetViewPortState();

        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
        };
        m_pipeline->SetDynamicState(dynamicStates);

        m_pipeline->SetRasterizerInfo(shift::info::CreateRasterStateInfo());
        m_pipeline->SetDepthStencilInfo(shift::info::CreateDepthStencilStateInfo());

        m_pipeline->SetMultisampleInfo(shift::info::CreateMultisampleStateInfo());

        auto blendState = shift::info::CreateBlendAttachmentState();
        m_pipeline->SetBlendAttachment(blendState);
        m_pipeline->SetBlendState(shift::info::CreateBlendStateInfo(blendState));

        VkFormat colF = m_backBuffer.swapchain->GetFormat();
        VkFormat depthF = m_backBuffer.swapchain->GetDepthBufferFormat();
        VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &colF;
        pipelineRenderingCreateInfo.depthAttachmentFormat = depthF;

        m_pipeline->SetDynamicRenderingInfo(pipelineRenderingCreateInfo);

        std::array<VkDescriptorSetLayout, 3> sets{
                m_descriptorManager->GetPerFrameLayout().Get(),
                m_descriptorManager->GetPerViewLayout(ViewSetLayoutType::DEFAULT_CAMERA).Get(),
                m_descriptorManager->GetPerMaterialLayout(MaterialSetLayoutType::TEXTURED).Get()
            };
        if (!m_pipeline->BuildLayout(sets)) {}

        m_pipeline->Build();
    }

    void Renderer::TempRecordCommandBuffer(const shift::gfx::CommandBuffer& cmdBuf, uint32_t imageIndex) {

        cmdBuf.TransferImageLayout(m_backBuffer.swapchain->GetImages()[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        auto colorAttInfo = info::CreateRenderingAttachmentInfo(m_backBuffer.swapchain->GetImageViews()[imageIndex]);
        auto depthAttInfo = info::CreateRenderingAttachmentInfo(m_backBuffer.swapchain->GetDepthBufferView(), false, {1.0f, 0});

        VkRenderingInfoKHR renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderInfo.renderArea = {.offset = {0, 0}, .extent = m_backBuffer.swapchain->GetExtent()};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = &colorAttInfo;
        renderInfo.pDepthAttachment = &depthAttInfo;

        cmdBuf.SetViewPort(m_backBuffer.swapchain->GetViewport());
        cmdBuf.SetScissor(m_backBuffer.swapchain->GetScissor());

        cmdBuf.BeginRendering(renderInfo);

        cmdBuf.BindPipeline(m_pipeline->Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);

        std::array<VkBuffer, 1> vertexBuffers{ m_amogus->GetVertexBufferPtr().Get() };
        std::array<VkDeviceSize, 1> offsets{ 0 };
        cmdBuf.BindVertexBuffers(vertexBuffers, offsets, 0);
        cmdBuf.BindIndexBuffer(m_amogus->GetIndexBufferRef().Get(), 0);

        // Bind the descriptor sets
        // INFO: The sets are not unique to pipelines, so we need to specify whether to bind it to compute pipeline or the graphics one
        std::array<VkDescriptorSet, 1> sets{ m_descriptorManager->GetPerFrameSet(m_currentFrame).Get() };
        cmdBuf.BindDescriptorSets(sets, {}, m_pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

        std::array<VkDescriptorSet, 1> setsView{ m_descriptorManager->GetPerViewSet(m_perViewID, m_currentFrame).Get() };
        cmdBuf.BindDescriptorSets(setsView, {}, m_pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

        std::array<VkDescriptorSet, 1> setsObj{ m_descriptorManager->GetPerMaterialSet(m_perMatID, m_currentFrame).Get() };
        cmdBuf.BindDescriptorSets(setsObj, {}, m_pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
        // DRAW THE FUCKING TRIANGLE
        cmdBuf.DrawIndexed(m_amogus->GetRanges()[0].indexNum, 1, 0, 0, 0);

        std::array<VkDescriptorSet, 1> setsObj2{ m_descriptorManager->GetPerMaterialSet(m_perMatID2, m_currentFrame).Get() };
        cmdBuf.BindDescriptorSets(setsObj2, {}, m_pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
        // DRAW THE FUCKING TRIANGLE
        cmdBuf.DrawIndexed(m_amogus->GetRanges()[0].indexNum, 1, 0, 0, 0);

        cmdBuf.EndRendering();

        cmdBuf.TransferImageLayout(m_backBuffer.swapchain->GetImages()[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        cmdBuf.EndCommandBuffer();
    }
} // shift::gfx
