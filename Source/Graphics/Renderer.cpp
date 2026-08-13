//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

#include "Graphics/RHI/Vulkan/VKImGuiBackend.hpp"

#include <glm/gtx/string_cast.hpp>

namespace Shift::Graphics {
    bool Renderer::Init(const std::optional<RHIRequiredFeatures>& featuresOverride) {

        const RHIRequiredFeatures requiredFeatures = featuresOverride.value_or(ShiftSelectedAPI::requiredFeatures);
        CheckCritical(m_renderBackend.Init(m_window.GetHandle(), m_window.GetWidth(), m_window.GetHeight(), "TestApp", "1.0.0", "Shift", "2.0.0", requiredFeatures), "Failed to initialize RHI!");

        RenderBackendInterface* rbi = m_renderBackend.CreateInterface();

        m_shaderManager.Init(rbi, Shift::Util::GetShiftShaderRootDir());
        m_pipelineManager.Init(&m_renderBackend, &m_shaderManager);
        m_bufferManager.Init(&m_renderBackend);

        CheckCritical(m_frameConstants.Init(m_bufferManager, "FrameConstantsRing"),
                      "Failed to create the frame constants ring!");

        RenderContext& tctx = m_renderBackend.GetTransferContext();
        RenderContextEncoder* tEncoder = tctx.CreateCommandEncoder();
        tctx.BeginCmds();
        tEncoder->PushDebugGroup("InitUploads", {0.85f, 0.55f, 0.20f, 1.0f});

        m_textureLoader = std::make_unique<StbLoader>();
        m_textureManager = std::make_unique<Graphics::TextureManager>(m_textureLoader.get(), &m_renderBackend, tctx.CreateCommandEncoder());

        m_textureManager->GetOrLoadTexture(Shift::Util::GetShiftRoot() + "Assets/Textures/NB.jpg", tEncoder);
        LoadScene();


        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor.name = "TrianglePipeline";
        ShaderDescriptor vsDescriptor;
        vsDescriptor.type = EShaderType::Vertex;
        vsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Debug/TriangleVS.slang";
        vsDescriptor.entry = "mainVS";
        ShaderDescriptor fsDescriptor;
        fsDescriptor.type = EShaderType::Fragment;
        fsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Debug/TrianglePS.slang";
        fsDescriptor.entry = "mainPS";

        pipelineDescriptor.colorBlendConfig.attachments.push_back({.format = ETextureFormat::B8G8R8A8_SRGB});

        pipelineDescriptor.pushConstants = PushConstantRange{
            .offset = 0,
            .size = static_cast<uint32_t>(sizeof(GPU::PushConstants)),
            .stageFlags = EBindingVisibility::Vertex | EBindingVisibility::Fragment
        };

        std::array<ShaderDescriptor, 2> shaderSources{vsDescriptor, fsDescriptor};
        m_pipeline = m_pipelineManager.CreatePipeline(pipelineDescriptor, shaderSources);

        uint32_t bufSize = 3 * sizeof(float) * 6;
        BufferDescriptor bufferDescriptor;
        bufferDescriptor.type = EBufferType::Staging;
        bufferDescriptor.name = "Stage";
        bufferDescriptor.size = bufSize;
        Buffer* staging = rbi->CreateBuffer(bufferDescriptor);

        BufferDescriptor bufferDescriptor2;
        bufferDescriptor2.type = EBufferType::Vertex;
        bufferDescriptor2.name = "TrianglePositions";
        bufferDescriptor2.size = bufSize;
        bufferDescriptor2.isDeviceAddressable = true;
        m_positionStream = m_bufferManager.CreateBuffer(bufferDescriptor2);

        CheckCritical(m_bufferManager.Get(m_positionStream)->GetDeviceAddress() != 0,
                      "Device-addressable buffer reported address 0 buffer device address is broken!");

        std::vector<float> vertexData = {
            0.5f,  0.5f, 0.5f,
            -0.0f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f,
            -0.5f,  0.5f, 0.5f,
            -0.5f, -0.5f, 0.5f,
            0.0f, -0.5f, 0.5f
        };

        staging->Fill(vertexData.data(), bufSize, 0);
        tctx.CreateCommandEncoder()->CopyBufferToBuffer({staging, 0}, {m_bufferManager.Get(m_positionStream), 0}, bufSize);

        {
            viewportSampler = rbi->CreateSampler(
                {
                    .minFilter = EFilterMode::Nearest,
                    .magFilter = EFilterMode::Nearest,
                    .name = "ViewportSampler"
                }
            );
        }

        {
            viewportTexture = rbi->CreateTexture(
            {
                    .width = m_window.GetWidth(),
                    .height = m_window.GetHeight(),
                    .format = ETextureFormat::B8G8R8A8_SRGB,
                    .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::Sampled,
                    .name = "ViewportRT"
                }
            );

        }
        //! First frame the viewport may be hidden so we immediately transfer this

        tEncoder->PopDebugGroup();
        tctx.EndCmds();

        std::array sigPayloads{m_renderBackend.ReserveTransferSignalPayload()};
        CheckCritical(tctx.SubmitCmds({}, sigPayloads), "Failed to submit transition context!");
        m_renderBackend.DeferExecute(sigPayloads[0].semaphore, sigPayloads[0].value, [staging]() mutable {
            delete staging;
        });

        RenderContext& gContext = m_renderBackend.GetGraphicsContext();
        gContext.BeginCmds();
        std::vector waitPayloads = m_renderBackend.FlushPendingAcquires(gContext);

        gContext.CreateCommandEncoder()->PushDebugGroup("TextureUploadFinalize", {0.90f, 0.80f, 0.30f, 1.0f});
        m_textureManager->UploadTexturesToGPU(gContext.CreateCommandEncoder());
        gContext.CreateCommandEncoder()->PopDebugGroup();

        gContext.EndCmds();
        waitPayloads.push_back(m_renderBackend.GetTransferWaitPayload());
        std::array sigPayloads2{m_renderBackend.ReserveGraphicsSignalPayload()};
        CheckCritical(gContext.SubmitCmds(waitPayloads, sigPayloads2, {}, {}), "Failed to submit graphics context!");

        m_renderBackend.WaitForGPU();
        m_textureManager->FreeStagingBuffers();

        return true;
    }

    void Renderer::RegisterViewportTexture() {
        m_viewportTextureID = ImGuiBackend::RegisterTexture(*viewportTexture, *viewportSampler);
    }

    bool Renderer::LoadScene() {
        return true;
    }

    bool Renderer::RenderFrame(const Shift::Graphics::EngineData &engineData, Editor::EditorLayer* editor) {

        //! The viewport may not be visible in the first frame and we need to transition the viewport texture for the imgui to render anyways
        bool firstFrame = m_renderBackend.GetCurrentGlobalIndex() == 0;
        bool shouldRenderMainViewport = true;
        bool shouldRenderMainWindow = m_window.GetWidth() > 0 && m_window.GetHeight() > 0;
        if (editor) {
            editor->BeginFrame();
            editor->Render(m_viewportTextureID);
            editor->EndFrame();
            shouldRenderMainViewport = editor->ShouldRenderViewportPanel() || firstFrame;
        }

        //! Pending uploads from the texture manager
        CheckCritical(m_textureManager->SubmitPendingUploads(), "Failed to submit pending texture uploads!");

        //! Reclaim this frame slot
        m_renderBackend.BeginFrame();

        const uint32_t frameSlot = m_renderBackend.GetCurrentFrame();
        GPU::FrameConstants* frameConstants = m_frameConstants.Slot(frameSlot);
        CheckCritical(frameConstants != nullptr, "Frame constants ring has no slot for this frame!");

        FillFrameConstants(frameConstants, engineData);

        uint32_t imageIndex = UINT32_MAX;
        if (shouldRenderMainWindow) {
            bool aquireSuccess = true;
            imageIndex = AquireImage(&aquireSuccess);
            if (imageIndex == UINT32_MAX) {
                return aquireSuccess;
            }
            m_renderBackend.WaitForImagePresent(imageIndex);
        }

        RenderContext& gContext = m_renderBackend.GetGraphicsContext();
        RenderContextEncoder* gEncoder = gContext.CreateCommandEncoder();

        gContext.ResetCmds();
        CheckCritical(gContext.BeginCmds(), "Failed to begin the command Buffer!");
        std::vector waitPayloads = m_renderBackend.FlushPendingAcquires(gContext);

        //! Register new uploads if any to GPU
        m_textureManager->RegisterSubmittedUploads();

        //! Outermost timed group
        gEncoder->PushDebugGroup("Frame", {0.55f, 0.45f, 0.85f, 1.0f}, true);

        if (shouldRenderMainViewport) {
            gEncoder->PushDebugGroup("ViewportPass", {0.30f, 0.65f, 0.35f, 1.0f}, true);
            // gContext.TransitionTexture(m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);
            gEncoder->TransitionTexture(*viewportTexture, EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

            RenderPassDescriptor renderPass;
            renderPass.colorAttachments.push_back(
                {
                    .renderTargetName = "ViewportTexture",
                    .clearValue = {.color = {0.3f, 0.3f, 0.3f, 1.0f}}
                }
            );
            renderPass.extent = {viewportTexture->GetWidth(), viewportTexture->GetHeight()};
            renderPass.enableSecondaryCommandBuffers = true;
            std::array colorTextures{viewportTexture};
            gEncoder->BeginRenderPass(renderPass, colorTextures, std::nullopt);
            // Reserve a single graphics signal payload and use it both for:
            // - telling the deferred executor when it's safe to free secondaries
            // - signalling from the primary submit
            auto graphicsSignal = m_renderBackend.ReserveGraphicsSignalPayload();

            // Acquire two secondary contexts (for current frame)
            RenderContext* sec0 = m_renderBackend.AcquireSecondaryGraphicsContext();
            RenderContext* sec1 = m_renderBackend.AcquireSecondaryGraphicsContext();

            Pipeline* pipeline = m_pipelineManager.Get(m_pipeline);

            std::vector<ETextureFormat> colorTexturesFormats{};
            for (auto& c: pipeline->GetDescriptor().colorBlendConfig.attachments) {
                colorTexturesFormats.push_back(c.format);
            }

            SecondaryBufferBeginPayload payload{
                .colorFormats = colorTexturesFormats
                // .depthFormat = pipeline->GetDescriptor().depthStencilConfig.depthFormat,
                // .stencilFormat = pipeline->GetDescriptor().depthStencilConfig.stencilFormat
            };

            if (sec0 && sec1) {
                // Record secondaries on two threads.
                auto record_secondary = [&](RHIContext<RHI::Vulkan>* sec) {
                    // NOTE: AcquireSecondaryGraphicsContext already called ResetCmds() on the context.
                    CheckCritical(sec->BeginSecondaryCmds(payload), "Failed to begin secondary command buffer!");

                    Rect2D scissor = {{0, 0}, {viewportTexture->GetWidth(), viewportTexture->GetHeight()}};
                    Viewport viewport = {0.0f, static_cast<float>(viewportTexture->GetHeight()), static_cast<float>(viewportTexture->GetWidth()), -static_cast<float>(viewportTexture->GetHeight()), 0.0f, 1.0f};

                    sec->CreateCommandEncoder()->SetScissor(scissor);
                    sec->CreateCommandEncoder()->SetViewport(viewport);

                    sec->CreateCommandEncoder()->BindGraphicsPipeline(*pipeline);

                    const GPU::PushConstants push{
                        .frameConstantsRef = m_frameConstants.SlotAddress(frameSlot),
                        .firstInstance = (sec == sec0) ? 0u : 1u
                    };
                    sec->CreateCommandEncoder()->SetPushConstants(*pipeline, &push, static_cast<uint32_t>(sizeof(push)));

                    sec->CreateCommandEncoder()->Draw({3, 1, 0, 0});

                    CheckCritical(sec->EndCmds(), "Failed to end secondary command buffer!");
                    return true;
                };

                std::thread th0(record_secondary, sec0);
                std::thread th1(record_secondary, sec1);

                // Wait for both recording threads to finish before executing them in primary
                th0.join();
                th1.join();

                // Execute the secondaries from the primary. Pass same graphicsSignal so
                // deferred executor will free them only after GPU signals it.
                std::array<RHIContext<RHI::Vulkan>*, 2> secondariesArr{sec0, sec1};
                m_renderBackend.ExecuteSecondaryGraphicsContexts(secondariesArr, graphicsSignal);
            }

            gEncoder->EndRenderPass();

            gEncoder->TransitionTexture(*viewportTexture, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
            gEncoder->PopDebugGroup();
        }

        if (shouldRenderMainWindow) {
            gEncoder->PushDebugGroup("UIPass", {0.35f, 0.55f, 0.90f, 1.0f}, true);
            // 1. Transition Swapchain to WRITE
            gEncoder->TransitionTexture(m_renderBackend.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

            // 2. Setup UI Render Pass
            RenderPassDescriptor uiPass;
            uiPass.colorAttachments.push_back({
                .renderTargetName = "SwapchainBackbuffer",
                .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
            });
            uiPass.extent = m_renderBackend.GetSwapchain().GetExtent();

            std::array swapchainImages{&m_renderBackend.GetSwapchain().GetSwapchainTexture(imageIndex)};
            gEncoder->BeginRenderPass(uiPass, swapchainImages, std::nullopt);

            if (editor) {
                ImGuiBackend::RenderDrawData(editor->GetDrawData(), gContext.GetCommandBuffer());
            }

            gEncoder->EndRenderPass();

            gEncoder->TransitionTexture(m_renderBackend.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::Present, EPipelineStageFlags::BottomOfPipeBit);
            gEncoder->PopDebugGroup();
        }

        gEncoder->PopDebugGroup(); //! Frame end

        CheckCritical(gContext.EndCmds(), "Failed to end the command Buffer!");

        waitPayloads.push_back(m_renderBackend.GetTransferWaitPayload());
        std::array sigPayloads{m_renderBackend.ReserveGraphicsSignalPayload()};
        if (shouldRenderMainWindow) {
            std::array imgAcquirePayload{m_renderBackend.GetSwapchainAcquireSemaphore(m_renderBackend.GetCurrentFrame())};
            std::array renderFinishedPayload{m_renderBackend.GetSwapchainRenderFinishedSemaphore(imageIndex)};
            CheckCritical(gContext.SubmitCmds(waitPayloads, sigPayloads, imgAcquirePayload, renderFinishedPayload), "Failed to submit graphics context!");
            CheckCritical(PresentFinalImage(imageIndex), "Failed to present final image!");
        } else {
            CheckCritical(gContext.SubmitCmds(waitPayloads, sigPayloads, {}, {}), "Failed to submit graphics context!");
        }


        //! TODO: Feature: Imgui floating window do not match vulkan's new
        // if (editor) {
        //     editor->RenderFloatingViewPorts();
        // }

        m_renderBackend.EndFrame();

        return true;
    }

    uint32_t Renderer::HotReloadShaders() {
        return m_pipelineManager.HotReload();
    }

    void Renderer::WaitForCleanup() {
        m_renderBackend.WaitForGPU();
    }

    void Renderer::Cleanup() {
        //! Drain the deferred queue while every resource is still alive so no queued callback
        m_renderBackend.FlushAllDeferredCallbacks();

        delete viewportTexture;
        delete viewportSampler;

        m_bufferManager.Destroy();
        m_pipelineManager.Destroy();
        m_shaderManager.Destroy();

        m_textureManager.reset();
        m_textureLoader.reset();
        m_renderBackend.Destroy();
    }

    void Renderer::ResizeViewport(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) return;
        if (width == viewportTexture->GetWidth() && height == viewportTexture->GetHeight()) return;

        //! Follow the viewport and not full window
        m_controller->UpdateScreenSize(static_cast<float>(width), static_cast<float>(height));

        Texture* oldTexture = viewportTexture;
        void* oldID = m_viewportTextureID;

        viewportTexture = m_renderBackend.CreateInterface()->CreateTexture({
            .width = width,
            .height = height,
            .format = ETextureFormat::B8G8R8A8_SRGB,
            .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::Sampled,
            .name = "ViewportRT"
        });

        RegisterViewportTexture();

        //! Destruction deferred to current frame + MAX FIF because prev frames might have already submitted write commands to viewport before resize
        m_renderBackend.DeferExecuteToFrame(m_renderBackend.GetCurrentGlobalIndex() + Conf::SHIFT_MAX_FRAMES_IN_FLIGHT, [oldTexture, oldID]() mutable {
            if (oldID) {
                ImGuiBackend::UnregisterTexture(oldID);
            }
            delete oldTexture;
        });
    }

    bool Renderer::PresentFinalImage(uint32_t imageIndex) {
        bool isOld = false;
        bool success = m_renderBackend.SwapchainPresent(imageIndex, &isOld);
        if (!success) { return false; }

        if (isOld || m_window.ShouldProcessResize()) {
            m_window.ProcessResize();
            if (!m_renderBackend.ResizeSwapchain(m_window.GetWidth(), m_window.GetHeight())) { return false; }
        }

        return true;
    }

    void Renderer::FillFrameConstants(GPU::FrameConstants *frameConstantsPtr, const EngineData &engineData) {
        frameConstantsPtr->cameraPosExposure = glm::vec4(engineData.camPosition, 1.0f);
        frameConstantsPtr->view = engineData.viewMatrix;
        frameConstantsPtr->proj = engineData.projMatrix;
        frameConstantsPtr->viewProj = engineData.projMatrix * engineData.viewMatrix;
        frameConstantsPtr->cameraDir = glm::vec4(engineData.camDirection, 0.0f);
        frameConstantsPtr->lightCount = 0u;
        frameConstantsPtr->positionsRef = m_bufferManager.Get(m_positionStream)->GetDeviceAddress();
    }

    uint32_t Renderer::AquireImage(bool *success) {
        bool changed = false;
        uint32_t imageIndex = m_renderBackend.SwapchainAquireImage(&changed);

        if (changed) {
            if (!m_renderBackend.ResizeSwapchain(m_window.GetWidth(), m_window.GetHeight())) {
                *success = false;
            }
            return UINT32_MAX;
        }

        if (imageIndex == UINT32_MAX) {
            *success = false;
        }

        return imageIndex;
    }

} // shift::gfx
