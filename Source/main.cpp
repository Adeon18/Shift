// #include <vulkan/vulkan.h> This is fine for offscreen rendering but for window rendering we do as below

#include "VertexStructures.hpp"
#include "UniformBufferStructs.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <iomanip>
#include <map>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Utility/UtilStandard.hpp"
#include "Utility/Vulkan/UtilVulkan.hpp"
#include "Utility/Vulkan/InfoUtil.hpp"

#include "Window/ShiftWindow.hpp"
#include "Graphics/Abstraction/Device/WindowSurface.hpp"
#include "Graphics/Abstraction/Device/Instance.hpp"
#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Device/Swapchain.hpp"
#include "Graphics/Abstraction/Synchronization/Fence.hpp"
#include "Graphics/Abstraction/Synchronization/Semaphore.hpp"
#include "Graphics/Abstraction/Commands/CommandPool.hpp"
#include "Graphics/Abstraction/Commands/CommandBuffer.hpp"
#include "Graphics/Abstraction/RenderPass/RenderPass.hpp"
#include "Graphics/Abstraction/Pipeline/Shader.hpp"
#include "Graphics/Abstraction/Pipeline/Pipeline.hpp"
#include "Graphics/Abstraction/Descriptors/DescriptorManager.hpp"
#include "Graphics/Abstraction/Buffers/BasicBuffers.hpp"
#include "Graphics/Abstraction/Images/Images.hpp"
#include "Input/Controllers/Camera/FlyingCameraController.hpp"

#include "Graphics/Systems/TextureSystem.hpp"

#include <spdlog/spdlog.h>

using namespace sft;

class HelloTriangleApplication {
    static constexpr uint32_t WINDOW_WIDTH = 800;
    static constexpr uint32_t WINDOW_HEIGHT = 600;

#ifdef NDEBUG
    static constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
    static constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

public:
    void run() {
        initWindow();
        if (!initVulkan()) {
            cleanup();
            return;
        }
        mainLoop();
        cleanup();
    }

private:

private:
    void initWindow() {
        m_winPtr = std::make_unique<sft::ShiftWindow>(WINDOW_WIDTH, WINDOW_HEIGHT, "Shift");
        spdlog::set_level(spdlog::level::debug);
    }

    bool initVulkan() {
        createVkInstance();

        createSurface();

        m_device = std::make_unique<sft::gfx::Device>(*m_instance, m_surfacePtr->Get());

        if (!createSwapchain()) {return false;}

        // MUST BE CREATED BEFORE PIPELINE
        if (!createRenderPass()) { return false; }
        createFramebuffers();
        createCommandPools();

        CreateTextureSystem();

        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        if (!createDescriptorPool()) {return false;}
        if (!createDescriptorSets()) {return false;}

        if (!createGraphicsPipeline()) {return false;}

        createSyncObjects();

        createCamera();
        return true;
    }

    //! Basically wer make an app info struct
    void createVkInstance() {
        m_instance = std::make_unique<sft::gfx::Instance>("TestApp", VK_MAKE_VERSION(1, 0, 0), "ShiftEngine",
                                                          VK_MAKE_VERSION(1, 0, 0));
    }

    void createSurface() {
        m_surfacePtr = std::make_unique<sft::gfx::WindowSurface>(m_instance->Get(), m_winPtr->GetHandle());
    }

    bool createSwapchain() {
        m_swapchain = std::make_unique<sft::gfx::Swapchain>(*m_device, *m_surfacePtr, m_winPtr->GetWidth(), m_winPtr->GetHeight());
        return m_swapchain->IsValid();
    }

    //! INFO WARNING UTILITY TODO We don't recreate the renderpass here, even though we should because the windoe might be moving to another
    //! screen which might be HDR so the image format might change
    bool recreateSwapChain() {
        vkDeviceWaitIdle(m_device->Get());
        // Wait for device to stop using resources
        cleanupSwapChain();

        if (!m_swapchain->Recreate(m_winPtr->GetWidth(), m_winPtr->GetHeight())) return false;

        createFramebuffers();
        return true;
    }

    void cleanupSwapChain() {
        for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(m_device->Get(), m_swapChainFramebuffers[i], nullptr);
        }
    }


    bool createGraphicsPipeline() {
        sft::gfx::Shader vert{*m_device, sft::util::GetShiftRoot() + "Shaders/shader.vert.spv", sft::gfx::Shader::Type::Vertex};
        if (!vert.CreateStage()) {return false;}

        sft::gfx::Shader frag{*m_device, sft::util::GetShiftRoot() + "Shaders/shader.frag.spv", sft::gfx::Shader::Type::Fragment};
        if (!frag.CreateStage()) {return false;}

        m_pipeline = std::make_unique<sft::gfx::Pipeline>(*m_device);

        m_pipeline->AddShaderStage(vert);
        m_pipeline->AddShaderStage(frag);

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        m_pipeline->SetInputStateInfo(sft::info::CreateInputStateInfo(attributeDescriptions, {&bindingDescription, 1}),VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        m_pipeline->SetViewPortState();

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        m_pipeline->SetDynamicState(dynamicStates);

        m_pipeline->SetRasterizerInfo(sft::info::CreateRasterStateInfo());

        m_pipeline->SetMultisampleInfo(sft::info::CreateMultisampleStateInfo());

        auto blendState = sft::info::CreateBlendAttachmentState();
        m_pipeline->SetBlendAttachment(blendState);
        m_pipeline->SetBlendState(sft::info::CreateBlendStateInfo(blendState));

        if (!m_pipeline->BuildLayout({m_descriptorManager->GetPerFrameSet(m_currentFrame).GetLayout().Ptr(), 1})) {return false;}

        return m_pipeline->Build(*m_renderPass);
    }

    bool createRenderPass() {
        m_renderPass = std::make_unique<sft::gfx::RenderPass>(*m_device);

        sft::gfx::Attachment att{m_swapchain->GetFormat(), sft::gfx::Attachment::Type::Swapchain, 0};

        sft::gfx::Subpass sub;
        sub.AddDependency(att);
        sub.BuildDescription();

        m_renderPass->AddAttachment(att);
        m_renderPass->AddSubpass(sub);

        return m_renderPass->BuildPass();
    }

    void createFramebuffers() {
        auto& swapImgViews = m_swapchain->GetImageViews();
        m_swapChainFramebuffers.resize(swapImgViews.size());

        for (size_t i = 0; i < swapImgViews.size(); i++) {
            VkImageView attachments[] = {
                    swapImgViews[i]
            };

            //! INFO: YOU CAN ONLY USE THE FRAMEBUFFER WITH THE COMPATIBLE RENDERPASS
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass->Get();
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_swapchain->GetExtent().width;
            framebufferInfo.height = m_swapchain->GetExtent().height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device->Get(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPools() {
        m_graphicsPool = std::make_unique<sft::gfx::CommandPool>(*m_device, sft::gfx::POOL_TYPE::GRAPHICS);
        m_transferPool = std::make_unique<sft::gfx::CommandPool>(*m_device, sft::gfx::POOL_TYPE::TRANSFER);
    }

    void CreateTextureSystem() {
        m_textureSystem = std::make_unique<sft::gfx::TextureSystem>(*m_device, *m_graphicsPool, *m_transferPool);

        m_textureIDs.push_back(m_textureSystem->LoadTexture(sft::util::GetShiftRoot() + "Assets/skeleton.png", VK_FORMAT_R8G8B8A8_SRGB));
        m_textureSystem->GetTexture(m_textureIDs[0])->CreateSampler(sft::info::CreateSamplerInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));

        m_textureIDs.push_back(m_textureSystem->LoadTexture(sft::util::GetShiftRoot() + "Assets/nobitches.jpg", VK_FORMAT_R8G8B8A8_SRGB));
        m_textureSystem->GetTexture(m_textureIDs[1])->CreateSampler(sft::info::CreateSamplerInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    }

    void createVertexBuffer() {
        
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        m_vertexBuffer = std::make_unique<sft::gfx::VertexBuffer>(*m_device, bufferSize);

        sft::gfx::StagingBuffer stagingBuffer{*m_device, bufferSize};

        memcpy(stagingBuffer.GetMappedBuffer(), vertices.data(), static_cast<size_t>(bufferSize));

        auto& buffer = m_transferPool->RequestCommandBuffer();
        buffer.CopyBuffer(stagingBuffer.Get(), m_vertexBuffer->Get(), bufferSize);
        buffer.EndCommandBuffer();
        buffer.SubmitAndWait();
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        sft::gfx::StagingBuffer stagingBuffer{*m_device, bufferSize};
        stagingBuffer.Fill(indices.data(), static_cast<size_t>(bufferSize));

        m_indexBuffer = std::make_unique<sft::gfx::IndexBuffer>(*m_device, bufferSize);

        auto& buffer = m_transferPool->RequestCommandBuffer();
        buffer.CopyBuffer(stagingBuffer.Get(), m_indexBuffer->Get(), bufferSize);
        buffer.EndCommandBuffer();
        buffer.SubmitAndWait();
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(PerFrame);
        m_uniformBuffers.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffers[i] = std::make_unique<sft::gfx::UniformBuffer>(*m_device, bufferSize);
        }
    }

    bool createDescriptorPool() {
        m_descriptorManager = std::make_unique<sft::gfx::DescriptorManager>(*m_device);
        return m_descriptorManager->AllocatePools();
    }

    bool createDescriptorSets() {
        for (uint32_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            auto& perFrameSet = m_descriptorManager->CreatePerFrameSet(i);

            perFrameSet.GetLayout().AddUBOBinding(0, VK_SHADER_STAGE_VERTEX_BIT);
            perFrameSet.GetLayout().AddSamplerBinding(1, VK_SHADER_STAGE_FRAGMENT_BIT);
            perFrameSet.GetLayout().Build();
        }

        if (!m_descriptorManager->AllocateAll()) { return false; }

        for (uint32_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            auto& perFrameSet = m_descriptorManager->GetPerFrameSet(i);

            auto* tex = m_textureSystem->GetTexture(m_textureIDs[0]);

            perFrameSet.UpdateUBO<PerFrame>(0, m_uniformBuffers[i]->Get(), 0);
            perFrameSet.UpdateImage(1, tex->GetView(), tex->GetSampler());
            perFrameSet.ProcessUpdates();
        }

        return true;
    }

    void recordCommandBuffer(const sft::gfx::CommandBuffer& cmdBuf, uint32_t imageIndex) {

        //! Begin a render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass->Get();
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapchain->GetExtent();

        VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        cmdBuf.BeginRenderPass(renderPassInfo);

        cmdBuf.BindPipeline(m_pipeline->Get(), VK_PIPELINE_BIND_POINT_GRAPHICS);

        std::array<VkBuffer, 1> vertexBuffers{ m_vertexBuffer->Get() };
        std::array<VkDeviceSize, 1> offsets{ 0 };
        cmdBuf.BindVertexBuffers(vertexBuffers, offsets, 0);
        cmdBuf.BindIndexBuffer(m_indexBuffer->Get(), 0);

        // Since the viewport and scissor are dynamic, we must set them here
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapchain->GetExtent().width);
        viewport.height = static_cast<float>(m_swapchain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmdBuf.SetViewPort(viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchain->GetExtent();
        cmdBuf.SetScissor(scissor);

        // Bind the descriptor sets
        // INFO: The sets are not unique to pipelines, so we need to specify whether to bind it to compute pipeline or the graphics one
        std::array<VkDescriptorSet, 1> sets{ m_descriptorManager->GetPerFrameSet(m_currentFrame).Get() };
        cmdBuf.BindDescriptorSets(sets, {}, m_pipeline->GetLayout(), VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

        // DRAW THE FUCKING TRIANGLE
        cmdBuf.DrawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        cmdBuf.EndRenderPass();

        cmdBuf.EndCommandBuffer();
    }

    void createSyncObjects() {
        m_imageAvailableSemaphores.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i] = std::make_unique<sft::gfx::Semaphore>(*m_device);
            m_renderFinishedSemaphores[i] = std::make_unique<sft::gfx::Semaphore>(*m_device);
        }
    }

    void mainLoop() {
        while (m_winPtr->IsActive()) {
            m_winPtr->Process();
            m_cameraController.CaptureInputAndApply();
            if (!drawFrame()) break;
            inp::Keyboard::GetInstance().UpdateKeys();
            inp::Mouse::GetInstance().UpdatePos();
        }

        //! Wait for device to finish operations so we can clean everything properly
        vkDeviceWaitIdle(m_device->Get());
    }

    bool drawFrame() {

        auto& buff = m_graphicsPool->RequestCommandBuffer(sft::gfx::BUFFER_TYPE::FLIGHT, m_currentFrame);

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
            auto &perFrameSet = m_descriptorManager->GetPerFrameSet(m_currentFrame);

            auto* tex = m_textureSystem->GetTexture(m_textureIDs[index]);

            //perFrameSet.UpdateUBO<PerFrame>(0, m_uniformBuffers[i], 0);
            perFrameSet.UpdateImage(1, tex->GetView(), tex->GetSampler());
            perFrameSet.ProcessUpdates();
            framesUpdated++;
        }
        ////////////////

        /// Aquire availible swapchain image index
        bool changed = false;
        uint32_t imageIndex = m_swapchain->AquireNextImageIndex(*m_imageAvailableSemaphores[m_currentFrame], &changed);
        if (imageIndex == UINT32_MAX) return false;
        if (changed) {
            if (!recreateSwapChain()) return false;
            return true;
        }

        recordCommandBuffer(buff, imageIndex);

        updateUniformBuffer(m_currentFrame);

        std::array<VkSemaphore, 1> waitSem{ m_imageAvailableSemaphores[m_currentFrame]->Get() };
        std::array<VkSemaphore, 1> sigSem{ m_renderFinishedSemaphores[m_currentFrame]->Get() };
        std::array<VkCommandBuffer, 1> cmdBuf{ buff.Get() };
        std::array<VkPipelineStageFlags, 1> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        buff.Submit(sft::info::CreateSubmitInfo(
                    waitSem,
                    sigSem,
                    cmdBuf,
                    waitStages.data()
                ));

        bool isOld = false;
        bool success = m_swapchain->Present(*m_renderFinishedSemaphores[m_currentFrame], imageIndex, &isOld);
        if (!success) {return false;}

        if (isOld || m_winPtr->ShouldProcessResize()) {
            m_winPtr->ProcessResize();
            m_cameraController.UpdateScreenSize(m_winPtr->GetWidth(), m_winPtr->GetHeight());
            if (!recreateSwapChain()) return false;
        }

        // Update the current frame
        m_currentFrame = (m_currentFrame + 1) % sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT;

        return true;
    }

    void updateUniformBuffer(uint32_t currentImage) {

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        PerFrame pf{};
        pf.model = glm::mat4(1.0f);

        //pf.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        pf.view = m_cameraController.GetCamera().GetViewMatrix();

        //pf.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_swapchain->GetExtent().width) / static_cast<float>(m_swapchain->GetExtent().height), 0.1f, 100.0f);
        pf.proj = m_cameraController.GetCamera().GetProjectionMatrix();

        // Y coordinate is fliped in Vulkan??
        pf.proj[1][1] *= -1;

        m_uniformBuffers[currentImage]->Fill(&pf, sizeof(pf));

//        std::cout << "Front: " << glm::to_string(m_cameraController.GetDirection() ) << std::endl;
//        std::cout << "Side: " << glm::to_string(m_cameraController.GetRightDir() ) << std::endl;
//        std::cout << "UP: " << glm::to_string(m_cameraController.GetUpDir() ) << std::endl;
    }

    void createCamera() {
        m_cameraController = ctrl::FlyingCameraController{90.0f, {m_winPtr->GetWidth(), m_winPtr->GetHeight()}, glm::vec3(0.0f, 0.0f, 2.0f)};
        m_cameraController.GetCamera().AddRotation({glm::pi<float>() / 6.0f, glm::pi<float>() / 6.0f, 0.0f});
    }

    void cleanup() {

        cleanupSwapChain();
        m_swapchain.reset();

        m_textureSystem.reset();

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_uniformBuffers[i].reset();
        }

        m_descriptorManager.reset();

        m_indexBuffer.reset();
        m_vertexBuffer.reset();

        for (size_t i = 0; i < sft::gutil::SHIFT_MAX_FRAMES_IN_FLIGHT; i++) {
            m_imageAvailableSemaphores[i].reset();
            m_renderFinishedSemaphores[i].reset();
        }
        m_graphicsPool.reset();
        m_transferPool.reset();

        m_pipeline.reset();

        m_renderPass.reset();

        m_surfacePtr.reset();
        m_device.reset();
    }
private:
    ctrl::FlyingCameraController m_cameraController;

    std::vector<sft::SGUID> m_textureIDs;

    std::unique_ptr<sft::gfx::TextureSystem> m_textureSystem;

    std::unique_ptr<sft::gfx::DescriptorManager> m_descriptorManager;

    std::unique_ptr<sft::ShiftWindow> m_winPtr;
    std::unique_ptr<sft::gfx::Device> m_device;
    std::unique_ptr<sft::gfx::WindowSurface> m_surfacePtr;
    std::unique_ptr<sft::gfx::Instance> m_instance;
    std::unique_ptr<sft::gfx::Swapchain> m_swapchain;

    std::unique_ptr<sft::gfx::CommandPool> m_graphicsPool;
    std::unique_ptr<sft::gfx::CommandPool> m_transferPool;

    std::unique_ptr<sft::gfx::RenderPass> m_renderPass;

    std::unique_ptr<sft::gfx::Pipeline> m_pipeline;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    std::unique_ptr<sft::gfx::VertexBuffer> m_vertexBuffer;
    std::unique_ptr<sft::gfx::IndexBuffer> m_indexBuffer;

    // We need a couple of uniform buffers because we have multiple frames in fright so that we do not write to a frame that
    // the GPU is reading from currently
    std::vector<std::unique_ptr<sft::gfx::UniformBuffer>> m_uniformBuffers;

    // Sync primitives to comtrol the rendering of a frame
    std::vector<std::unique_ptr<sft::gfx::Semaphore>> m_imageAvailableSemaphores;
    std::vector<std::unique_ptr<sft::gfx::Semaphore>> m_renderFinishedSemaphores;

    uint32_t m_currentFrame = 0;
};

int main() {
    HelloTriangleApplication app;

    app.run();

    return EXIT_SUCCESS;
}