//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

#include <glm/gtx/string_cast.hpp>

namespace Shift::gfx {
    bool Renderer::Init() {

        CheckCritical(m_SRHI.Init(m_window.GetHandle(), m_window.GetWidth(), m_window.GetHeight(), "TestApp", "1.0.0", "Shift", "2.0.0"), "Failed to initialize RHI!");

        LoadScene();

        PipelineDescriptor pipelineDescriptor;
        ShaderDescriptor vsDescriptor;
        vsDescriptor.type = EShaderType::Vertex;
        vsDescriptor.path = Util::GetShiftShaderBuildDir() + "ConstantColor.vert.spv";
        ShaderDescriptor fsDescriptor;
        fsDescriptor.type = EShaderType::Fragment;
        fsDescriptor.path = Util::GetShiftShaderBuildDir() + "ConstantColor.frag.spv";

        vs = m_SRHI.CreateShader(vsDescriptor);
        ps = m_SRHI.CreateShader(fsDescriptor);

        std::vector<ShaderStageDesc> stages{
                {EShaderType::Vertex, &vs},
                {EShaderType::Fragment, &ps},
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

        return true;
    }

    bool Renderer::LoadScene() {
        return true;
    }

    bool Renderer::RenderFrame(const Shift::gfx::EngineData &engineData) {

        m_SRHI.WaitForGraphicsContext();

        SRHIContext gContext = m_SRHI.GetGraphicsContext();
        gContext.ResetCmds();
        CheckCritical(gContext.BeginCmds(), "Failed to begin the command Buffer!");
        /// Aquire availible swapchain image index

        bool aquireSuccess = true;
        uint32_t imageIndex = AquireImage(&aquireSuccess);
        if (imageIndex == UINT32_MAX) { return aquireSuccess; }

        gContext.TransitionTexture(m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

        gContext.SetScissor(m_SRHI.GetSwapchain().GetScissor());
        gContext.SetViewport(m_SRHI.GetSwapchain().GetViewport());
        RenderPassDescriptor renderPass;
        renderPass.colorAttachments.push_back(
            {
                .renderTargetName = "SwapchainBackbuffer",
                .clearValue = {.color = {0.3f, 0.3f, 0.3f, 1.0f}}
            }
        );
        renderPass.extent = m_SRHI.GetSwapchain().GetExtent();
        renderPass.enableSecondaryCommandBuffers = true;
        std::array colorTextures{&m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex)};
        gContext.BeginRenderPass(renderPass, colorTextures, std::nullopt);
        // Reserve a single graphics signal payload and use it both for:
        // - telling the deferred executor when it's safe to free secondaries
        // - signalling from the primary submit
        auto graphicsSignal = m_SRHI.ReserveGraphicsSignalPayload();

        // Acquire two secondary contexts (for current frame)
        SRHIContext* sec0 = m_SRHI.AcquireSecondaryGraphicsContext();
        SRHIContext* sec1 = m_SRHI.AcquireSecondaryGraphicsContext();

        std::vector<ETextureFormat> colorTexturesFormats{};
        for (auto& c: p.GetDescriptor().colorBlendConfig.attachments) {
            colorTexturesFormats.push_back(c.format);
        }

        SecondaryBufferBeginPayload payload{
            .colorFormats = colorTexturesFormats,
            .depthFormat = p.GetDescriptor().depthStencilConfig.depthFormat,
            .stencilFormat = p.GetDescriptor().depthStencilConfig.stencilFormat
        };

        if (sec0 && sec1) {
            // Record secondaries on two threads.
            auto record_secondary = [&](RHIContext<RHI::Vulkan>* sec) {
                // NOTE: AcquireSecondaryGraphicsContext already called ResetCmds() on the context.
                CheckCritical(sec->BeginSecondaryCmds(payload), "Failed to begin secondary command buffer!");

                // Set scissor/viewport on the secondary so draw matches primary state
                sec->SetScissor(m_SRHI.GetSwapchain().GetScissor());
                sec->SetViewport(m_SRHI.GetSwapchain().GetViewport());

                // Bind pipeline / vertex buffer and issue a draw (triangle)
                sec->BindGraphicsPipeline(p);
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

        gContext.TransitionTexture(m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex), EResourceLayout::Present, EPipelineStageFlags::BottomOfPipeBit);

        CheckCritical(gContext.EndCmds(), "Failed to end the command Buffer!");

        std::array waitPayloads{m_SRHI.GetTransferWaitPayload()};
        std::array sigPayloads{m_SRHI.ReserveGraphicsSignalPayload()};
        std::array imgAcquirePayload{m_SRHI.GetSwapchainAcquireSemaphore(m_SRHI.GetCurrentFrame())};
        std::array renderFinishedPayload{m_SRHI.GetSwapchainRenderFinishedSemaphore(imageIndex)};
        CheckCritical(gContext.SubmitCmds(waitPayloads, sigPayloads, imgAcquirePayload, renderFinishedPayload), "Failed to submit graphics context!");

        CheckCritical(PresentFinalImage(imageIndex), "Failed to present final image!");

        m_SRHI.ProcessDeferredCallbacks();

        // Update the current frame
        m_SRHI.EndFrame();

        return true;
    }

    void Renderer::Cleanup() {
        m_SRHI.WaitForGPU();
        p.Destroy();
        vs.Destroy();
        ps.Destroy();
        vertex.Destroy();
        m_SRHI.Destroy();
    }

    bool Renderer::PresentFinalImage(uint32_t imageIndex) {
        bool isOld = false;
        bool success = m_SRHI.SwapchainPresent(imageIndex, &isOld);
        if (!success) { return false; }

        if (isOld || m_window.ShouldProcessResize()) {
            m_window.ProcessResize();
            m_controller->UpdateScreenSize(static_cast<float>(m_window.GetWidth()), static_cast<float>(m_window.GetHeight()));
            if (!m_SRHI.GetSwapchain().Recreate(m_window.GetWidth(), m_window.GetHeight())) { return false; }
        }

        return true;
    }

    uint32_t Renderer::AquireImage(bool *success) {
        bool changed = false;
        uint32_t imageIndex = m_SRHI.SwapchainAquireImage(&changed);
        if (imageIndex == UINT32_MAX) {
            *success = false;
        } else if (changed) {
            if (!m_SRHI.GetSwapchain().Recreate(m_window.GetWidth(), m_window.GetHeight())) {
                *success = false;
            }
            return UINT32_MAX;
        }

        return imageIndex;
    }

} // shift::gfx
