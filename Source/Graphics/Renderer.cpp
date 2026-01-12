//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

#include "UI/ImGuiTools.hpp"

#include <glm/gtx/string_cast.hpp>

namespace Shift::gfx {
    bool Renderer::Init() {

        CheckCritical(m_SRHI.Init(m_window.GetHandle(), m_window.GetWidth(), m_window.GetHeight(), "TestApp", "1.0.0", "Shift", "2.0.0"), "Failed to initialize RHI!");

        LoadScene();

        PipelineDescriptor pipelineDescriptor;
        ShaderDescriptor vsDescriptor;
        vsDescriptor.type = EShaderType::Vertex;
        vsDescriptor.path = Util::GetShiftShaderSrcDir() + "Debug/TriangleVS.slang";
        vsDescriptor.entry = "mainVS";
        ShaderDescriptor fsDescriptor;
        fsDescriptor.type = EShaderType::Fragment;
        fsDescriptor.path = Util::GetShiftShaderSrcDir() + "Debug/TrianglePS.slang";
        fsDescriptor.entry = "mainPS";

        vs = m_SRHI.CreateShader(vsDescriptor);
        ps = m_SRHI.CreateShader(fsDescriptor);

        std::vector<ShaderStageDesc> stages{
                {EShaderType::Vertex, vs},
                {EShaderType::Fragment, ps},
            };

        pipelineDescriptor.vertexConfig.vertexBindings.emplace_back(
            0, 12, EVertexInputRate::PerVertex
        );
        pipelineDescriptor.vertexConfig.attributeDescs.emplace_back(
            0, 0, 0, EVertexAttributeFormat::R32G32B32_SignedFloat
        );
        pipelineDescriptor.colorBlendConfig.attachments.push_back({.format = ETextureFormat::B8G8R8A8_SRGB});

        p = m_SRHI.CreatePipeline(pipelineDescriptor, stages);

        uint32_t bufSize = 3 * sizeof(float) * 6;
        BufferDescriptor bufferDescriptor;
        bufferDescriptor.type = EBufferType::Staging;
        bufferDescriptor.name = "Stage";
        bufferDescriptor.size = bufSize;
        Buffer staging = m_SRHI.CreateBuffer(bufferDescriptor);

        BufferDescriptor bufferDescriptor2;
        bufferDescriptor2.type = EBufferType::Vertex;
        bufferDescriptor2.name = "Vertex";
        bufferDescriptor2.size = bufSize;
        vertex = m_SRHI.CreateBuffer(bufferDescriptor2);

        std::vector<float> vertexData = {
            0.5f,  0.5f, 0.5f,
            -0.0f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f,
            -0.5f,  0.5f, 0.5f,
            -0.5f, -0.5f, 0.5f,
            0.0f, -0.5f, 0.5f
        };

        SRHIContext tctx = m_SRHI.GetTransferContext();
        tctx.BeginCmds();
        staging.Fill(vertexData.data(), bufSize, 0);
        tctx.CopyBufferToBuffer({&staging, 0}, {&vertex, 0}, bufSize);
        tctx.EndCmds();

        std::array sigPayloads{m_SRHI.ReserveTransferSignalPayload()};
        CheckCritical(tctx.SubmitCmds({}, sigPayloads), "Failed to submit transition context!");
        m_SRHI.DeferExecute(sigPayloads[0].semaphore, sigPayloads[0].value, [staging]() mutable { staging.Destroy(); });

        {
            viewportSampler = m_SRHI.CreateSampler(
                {
                    .minFilter = EFilterMode::Nearest,
                    .magFilter = EFilterMode::Nearest
                }
            );
        }

        {
            viewportTexture = m_SRHI.CreateTexture(
            {
                    .width = m_window.GetWidth(),
                    .height = m_window.GetHeight(),
                    .format = ETextureFormat::B8G8R8A8_SRGB,
                    .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::Sampled,
                }
            );

        }

        return true;
    }

    void Renderer::RegisterViewportTexture() {
        m_viewportTextureID = RegisterTextureForImGui(&viewportSampler, &viewportTexture);
    }

    bool Renderer::LoadScene() {
        return true;
    }

    bool Renderer::RenderFrame(const Shift::gfx::EngineData &engineData, Editor::EditorLayer* editor) {

        bool shouldRenderMainViewport = true;
        bool shouldRenderMainWindow = m_window.GetWidth() > 0 && m_window.GetHeight() > 0;
        if (editor) {
            editor->BeginFrame();
            editor->Render(m_viewportTextureID);
            editor->EndFrame();
            shouldRenderMainViewport = editor->ShouldRenderViewportPanel();
        }

        //! Perform fence wait on presentation
        m_SRHI.WaitForGraphicsContext();

        uint32_t imageIndex = UINT32_MAX;
        if (shouldRenderMainWindow) {
            bool aquireSuccess = true;
            imageIndex = AquireImage(&aquireSuccess);
            if (imageIndex == UINT32_MAX) {
                return aquireSuccess;
            }
            m_SRHI.WaitForImagePresent(imageIndex);
        }

        SRHIContext gContext = m_SRHI.GetGraphicsContext();
        gContext.ResetCmds();
        CheckCritical(gContext.BeginCmds(), "Failed to begin the command Buffer!");

        if (shouldRenderMainViewport) {
            // gContext.TransitionTexture(m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);
            gContext.TransitionTexture(viewportTexture, EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

            RenderPassDescriptor renderPass;
            renderPass.colorAttachments.push_back(
                {
                    .renderTargetName = "ViewportTexture",
                    .clearValue = {.color = {0.3f, 0.3f, 0.3f, 1.0f}}
                }
            );
            renderPass.extent = {viewportTexture.GetWidth(), viewportTexture.GetHeight()};
            renderPass.enableSecondaryCommandBuffers = true;
            std::array colorTextures{&viewportTexture};
            gContext.BeginRenderPass(renderPass, colorTextures, std::nullopt);
            // Reserve a single graphics signal payload and use it both for:
            // - telling the deferred executor when it's safe to free secondaries
            // - signalling from the primary submit
            auto graphicsSignal = m_SRHI.ReserveGraphicsSignalPayload();

            // Acquire two secondary contexts (for current frame)
            SRHIContext* sec0 = m_SRHI.AcquireSecondaryGraphicsContext();
            SRHIContext* sec1 = m_SRHI.AcquireSecondaryGraphicsContext();

            std::vector<ETextureFormat> colorTexturesFormats{};
            for (auto& c: p->GetDescriptor().colorBlendConfig.attachments) {
                colorTexturesFormats.push_back(c.format);
            }

            SecondaryBufferBeginPayload payload{
                .colorFormats = colorTexturesFormats
                // .depthFormat = p->GetDescriptor().depthStencilConfig.depthFormat,
                // .stencilFormat = p->GetDescriptor().depthStencilConfig.stencilFormat
            };

            if (sec0 && sec1) {
                // Record secondaries on two threads.
                auto record_secondary = [&](RHIContext<RHI::Vulkan>* sec) {
                    // NOTE: AcquireSecondaryGraphicsContext already called ResetCmds() on the context.
                    CheckCritical(sec->BeginSecondaryCmds(payload), "Failed to begin secondary command buffer!");

                    Rect2D scissor = {{0, 0}, {viewportTexture.GetWidth(), viewportTexture.GetHeight()}};
                    Viewport viewport = {0.0f, static_cast<float>(viewportTexture.GetHeight()), static_cast<float>(viewportTexture.GetWidth()), -static_cast<float>(viewportTexture.GetHeight()), 0.0f, 1.0f};

                    sec->SetScissor(scissor);
                    sec->SetViewport(viewport);

                    sec->BindGraphicsPipeline(*p);
                    sec->BindVertexBuffer({&vertex, 0}, 0);
                    sec->Draw({3, 1, (sec == sec0) ? 0u: 3u, 0});

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
                m_SRHI.ExecuteSecondaryGraphicsContexts(secondariesArr, graphicsSignal);
            }

            gContext.EndRenderPass();

            gContext.TransitionTexture(viewportTexture, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
        }

        if (shouldRenderMainWindow) {
            // 1. Transition Swapchain to WRITE
            gContext.TransitionTexture(m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

            // 2. Setup UI Render Pass
            RenderPassDescriptor uiPass;
            uiPass.colorAttachments.push_back({
                .renderTargetName = "SwapchainBackbuffer",
                .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
            });
            uiPass.extent = m_SRHI.GetSwapchain().GetExtent();

            std::array swapchainImages{&m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex)};
            gContext.BeginRenderPass(uiPass, swapchainImages, std::nullopt);

            if (editor) {
                ImGuiRenderDrawData(editor->GetDrawData(), &gContext.GetCommandBuffer());
            }

            gContext.EndRenderPass();

            gContext.TransitionTexture(m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::Present, EPipelineStageFlags::BottomOfPipeBit);
        }

        CheckCritical(gContext.EndCmds(), "Failed to end the command Buffer!");

        std::array waitPayloads{m_SRHI.GetTransferWaitPayload()};
        std::array sigPayloads{m_SRHI.ReserveGraphicsSignalPayload()};
        if (shouldRenderMainWindow) {
            std::array imgAcquirePayload{m_SRHI.GetSwapchainAcquireSemaphore(m_SRHI.GetCurrentFrame())};
            std::array renderFinishedPayload{m_SRHI.GetSwapchainRenderFinishedSemaphore(imageIndex)};
            CheckCritical(gContext.SubmitCmds(waitPayloads, sigPayloads, imgAcquirePayload, renderFinishedPayload), "Failed to submit graphics context!");
            CheckCritical(PresentFinalImage(imageIndex), "Failed to present final image!");
        } else {
            CheckCritical(gContext.SubmitCmds(waitPayloads, sigPayloads, {}, {}), "Failed to submit graphics context!");
        }


        //! TODO: Feature: Imgui floating window do not match vulkan's new
        // if (editor) {
        //     editor->RenderFloatingViewPorts();
        // }

        m_SRHI.ProcessDeferredCallbacks();

        // Update the current frame
        m_SRHI.EndFrame();

        return true;
    }

    void Renderer::HotReloadShaders() {
        m_SRHI.ShaderHotReload();
    }

    void Renderer::WaitForCleanup() {
        m_SRHI.WaitForGPU();
    }

    void Renderer::Cleanup() {
        p->Destroy();
        vs->Destroy();
        ps->Destroy();
        vertex.Destroy();
        viewportTexture.Destroy();
        viewportSampler.Destroy();
        m_SRHI.Destroy();
    }

    void Renderer::ResizeViewport(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) return;
        if (width == viewportTexture.GetWidth() && height == viewportTexture.GetHeight()) return;

        Texture oldTexture = viewportTexture;
        void* oldID = m_viewportTextureID;

        viewportTexture = m_SRHI.CreateTexture({
            .width = width,
            .height = height,
            .format = ETextureFormat::B8G8R8A8_SRGB,
            .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::Sampled
        });

        m_viewportTextureID = RegisterTextureForImGui(
            &viewportSampler,
            &viewportTexture
        );

        //! Defer the end of next frame
        m_SRHI.DeferExecuteToFrame(m_SRHI.GetCurrentGlobalIndex()+1, [oldTexture, oldID]() mutable {
            if (oldID) {
                UnregisterTextureForImGui(oldID);
            }
            oldTexture.Destroy();
        });
    }

    bool Renderer::PresentFinalImage(uint32_t imageIndex) {
        bool isOld = false;
        bool success = m_SRHI.SwapchainPresent(imageIndex, &isOld);
        if (!success) { return false; }

        if (isOld || m_window.ShouldProcessResize()) {
            m_window.ProcessResize();
            m_controller->UpdateScreenSize(static_cast<float>(m_window.GetWidth()), static_cast<float>(m_window.GetHeight()));
            if (!m_SRHI.ResizeSwapchain(m_window.GetWidth(), m_window.GetHeight())) { return false; }
        }

        return true;
    }

    uint32_t Renderer::AquireImage(bool *success) {
        bool changed = false;
        uint32_t imageIndex = m_SRHI.SwapchainAquireImage(&changed);

        if (changed) {
            if (!m_SRHI.ResizeSwapchain(m_window.GetWidth(), m_window.GetHeight())) {
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
