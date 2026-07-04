//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

#include "Graphics/RHI/Vulkan/VKImGuiBackend.hpp"

#include <glm/gtx/string_cast.hpp>

namespace Shift::Graphics {
    bool Renderer::Init() {

        CheckCritical(m_renderBackend.Init(m_window.GetHandle(), m_window.GetWidth(), m_window.GetHeight(), "TestApp", "1.0.0", "Shift", "2.0.0"), "Failed to initialize RHI!");

        RenderBackendInterface* rbi = m_renderBackend.CreateInterface();

        m_shaderManager.Init(rbi, Shift::Util::GetShiftShaderRootDir());
        m_pipelineManager.Init(&m_renderBackend, &m_shaderManager);
        m_bufferManager.Init(&m_renderBackend);

        RenderContext& tctx = m_renderBackend.GetTransferContext();
        RenderContextEncoder* tEncoder = tctx.CreateCommandEncoder();
        tctx.BeginCmds();
        tEncoder->PushDebugGroup("InitUploads");

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

        pipelineDescriptor.vertexConfig.vertexBindings.emplace_back(
            0, 12, EVertexInputRate::PerVertex
        );
        pipelineDescriptor.vertexConfig.attributeDescs.emplace_back(
            0, 0, 0, EVertexAttributeFormat::R32G32B32_SignedFloat
        );
        pipelineDescriptor.colorBlendConfig.attachments.push_back({.format = ETextureFormat::B8G8R8A8_SRGB});

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
        bufferDescriptor2.name = "Vertex";
        bufferDescriptor2.size = bufSize;
        m_vertexBuffer = m_bufferManager.CreateBuffer(bufferDescriptor2);

        std::vector<float> vertexData = {
            0.5f,  0.5f, 0.5f,
            -0.0f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f,
            -0.5f,  0.5f, 0.5f,
            -0.5f, -0.5f, 0.5f,
            0.0f, -0.5f, 0.5f
        };

        staging->Fill(vertexData.data(), bufSize, 0);
        tctx.CreateCommandEncoder()->CopyBufferToBuffer({staging, 0}, {m_bufferManager.Get(m_vertexBuffer), 0}, bufSize);

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

        gContext.CreateCommandEncoder()->PushDebugGroup("TextureUploadFinalize");
        m_textureManager->UploadTexturesToGPU(gContext.CreateCommandEncoder());
        gContext.CreateCommandEncoder()->PopDebugGroup();

        gContext.EndCmds();
        std::array waitPayloads{m_renderBackend.GetTransferWaitPayload()};
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

        //! Perform fence wait on presentation
        m_renderBackend.WaitForGraphicsContext();

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

        if (shouldRenderMainViewport) {
            gEncoder->PushDebugGroup("ViewportPass");
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
            Buffer* vertexBuf = m_bufferManager.Get(m_vertexBuffer);

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
                    sec->CreateCommandEncoder()->BindVertexBuffer({vertexBuf, 0}, 0);
                    sec->CreateCommandEncoder()->Draw({3, 1, (sec == sec0) ? 0u: 3u, 0});

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
            gEncoder->PushDebugGroup("UIPass");
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

        CheckCritical(gContext.EndCmds(), "Failed to end the command Buffer!");

        std::array waitPayloads{m_renderBackend.GetTransferWaitPayload()};
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

        m_renderBackend.ProcessDeferredCallbacks();

        // Update the current frame
        m_renderBackend.EndFrame();

        return true;
    }

    void Renderer::HotReloadShaders() {
        m_pipelineManager.HotReload();
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

        //! Defer the end of next frame
        m_renderBackend.DeferExecuteToFrame(m_renderBackend.GetCurrentGlobalIndex()+1, [oldTexture, oldID]() mutable {
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
            m_controller->UpdateScreenSize(static_cast<float>(m_window.GetWidth()), static_cast<float>(m_window.GetHeight()));
            if (!m_renderBackend.ResizeSwapchain(m_window.GetWidth(), m_window.GetHeight())) { return false; }
        }

        return true;
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
