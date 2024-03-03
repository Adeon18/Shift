//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Graphics/Abstraction/Descriptors/UBOStructs.hpp"
#include "Utility/Vulkan/InfoUtil.hpp"

#include <glm/gtx/string_cast.hpp>

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

        m_descriptorManager = std::make_unique<DescriptorManager>(*m_context.device);
        if (!m_descriptorManager->AllocatePools()) {return false;}
        m_textureSystem = std::make_unique<TextureSystem>(*m_context.device, *m_context.graphicsPool, *m_context.transferPool);
        m_modelManager = std::make_unique<ModelManager>(*m_context.device, *m_context.transferPool, *m_textureSystem);
        m_bufferManager = std::make_unique<BufferManager>(*m_context.device);

        m_amogus = m_modelManager->LoadModel(shift::util::GetShiftRoot() + "Assets/Models/SimpleAmogusPink/scene.gltf");
        TempCreate();

        m_meshSystem = std::make_unique<MeshSystem>(*m_context.device, m_backBuffer, *m_textureSystem, *m_modelManager, *m_bufferManager, *m_descriptorManager);

        for (int i = -16; i < 16; ++i) {
            for (int j = -16; j < 16; ++j) {
                m_meshSystem->AddInstance(MeshPass::Textured, Mobility::STATIC, m_amogus,
                                          glm::translate(glm::mat4(1), glm::vec3(1.5f * i, 0.0f, 1.5f * j)));
            }
        }

        m_meshSystem->AddInstance(MeshPass::Emission, Mobility::STATIC, m_amogus,
                                  glm::translate(glm::mat4(1), glm::vec3(1.0f, 1.0f, 0.0f)));
    }

    bool Renderer::RenderFrame(const shift::gfx::EngineData &engineData) {
        auto& buff = m_context.graphicsPool->RequestCommandBuffer(shift::gfx::BUFFER_TYPE::FLIGHT, m_currentFrame);

        /// Aquire availible swapchain image index
        bool changed = false;
        uint32_t imageIndex = m_backBuffer.swapchain->AquireNextImageIndex(*m_imageAvailableSemaphores[m_currentFrame], &changed);
        if (imageIndex == UINT32_MAX) return false;
        if (changed) {
            if (!m_backBuffer.swapchain->Recreate(m_window.GetWidth(), m_window.GetHeight())) return false;
            return true;
        }

        buff.TransferImageLayout(m_backBuffer.swapchain->GetImages()[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

        m_meshSystem->RenderAllPasses(buff, imageIndex, m_currentFrame, m_perViewID);

        buff.TransferImageLayout(m_backBuffer.swapchain->GetImages()[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
        buff.EndCommandBuffer();

        /// BUffer updates

        PerDefaultView pf{};
        pf.view = engineData.viewMatrix;
        pf.proj = engineData.projMatrix;
        pf.viewInv = glm::inverse(engineData.viewMatrix);
        pf.projInv = glm::inverse(engineData.projMatrix);
        auto& b = m_bufferManager->GetUBO(m_perViewID, m_currentFrame);
        b.Fill(&pf, sizeof(pf));

        PerFrame pfr{};
        pfr.camDirection = glm::vec4{engineData.camDirection, 0};
        pfr.camPosition = glm::vec4{engineData.camPosition, 0};
        pfr.camUp = glm::vec4{engineData.camUp, 0};
        pfr.camRight = glm::vec4{engineData.camRight, 0};
        pfr.windowData = glm::vec4{static_cast<float>(engineData.winWidth), static_cast<float>(engineData.winHeight), engineData.oneDivWinWidth, engineData.oneDivWinHeight};
        pfr.timerData = glm::vec4{engineData.dt, engineData.fps, engineData.secondsSinceStart, 0};
        auto& b1 = m_bufferManager->GetUBO(m_perFrameID, m_currentFrame);
        b1.Fill(&pfr, sizeof(pfr));

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
        m_modelManager.reset();
        m_bufferManager.reset();

        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i].reset();
            m_renderFinishedSemaphores[i].reset();
        }
        m_meshSystem.reset();

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

            m_descriptorManager->CreatePerMaterialLayout(
                    MaterialSetLayoutType::EMISSION_ONLY,
                    {
                            {DescriptorType::UBO, 0, VK_SHADER_STAGE_VERTEX_BIT},
                    }
            );
        }
        m_perViewID = m_descriptorManager->AllocatePerViewSet(ViewSetLayoutType::DEFAULT_CAMERA);
        m_perFrameID = m_descriptorManager->AllocatePerFrameSet();

        VkDeviceSize bufferSize = sizeof(PerDefaultView);
        m_bufferManager->AllocateUBO(m_perViewID, bufferSize);
        bufferSize = sizeof(PerFrame);
        m_bufferManager->AllocateUBO(m_perFrameID, bufferSize);

        for (uint32_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            auto& perFrameSet = m_descriptorManager->GetPerFrameSet(i);
            auto& buff = m_bufferManager->GetUBO(m_perFrameID, i);
            perFrameSet.UpdateUBO(0, buff.Get(), 0, buff.GetSize());
            perFrameSet.ProcessUpdates();

            auto& perViewSet = m_descriptorManager->GetPerViewSet(m_perViewID, i);
            auto& buff2 = m_bufferManager->GetUBO(m_perViewID, i);
            perViewSet.UpdateUBO(0, buff2.Get(), 0, buff2.GetSize());
            perViewSet.ProcessUpdates();
        }

        m_imageAvailableSemaphores.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < shift::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i] = std::make_unique<shift::gfx::Semaphore>(*m_context.device);
            m_renderFinishedSemaphores[i] = std::make_unique<shift::gfx::Semaphore>(*m_context.device);
        }

    }
} // shift::gfx