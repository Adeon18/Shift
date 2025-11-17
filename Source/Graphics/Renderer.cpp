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

        uint32_t bufSize = 3 * sizeof(float) * 3;
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
            -0.5f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f
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
        std::array colorTextures{&m_SRHI.GetSwapchain().GetSwapchainTexture(imageIndex)};
        gContext.BeginRenderPass(renderPass, colorTextures, std::nullopt);

        gContext.BindGraphicsPipeline(p);

        gContext.BindVertexBuffer({&vertex, 0}, 0);


        gContext.Draw({3, 1, 0, 0});
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
        m_SRHI.NextFrame();

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
