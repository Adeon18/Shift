//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

#include "Graphics/RHI/Vulkan/VKImGuiBackend.hpp"
#include "Loaders/ModelLoader/GltfLoader.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Shift::Graphics {
    bool Renderer::Init(const std::optional<RHIRequiredFeatures>& featuresOverride) {

        const RHIRequiredFeatures requiredFeatures = featuresOverride.value_or(ShiftSelectedAPI::requiredFeatures);
        CheckCritical(m_renderBackend.Init(m_window.GetHandle(), m_window.GetWidth(), m_window.GetHeight(), "TestApp", "1.0.0", "Shift", "2.0.0", requiredFeatures), "Failed to initialize RHI!");

        RenderBackendInterface* rbi = m_renderBackend.CreateInterface();

        m_shaderManager.Init(rbi, Shift::Util::GetShiftShaderRootDir());
        m_pipelineManager.Init(&m_renderBackend, &m_shaderManager);
        m_bufferManager.Init(&m_renderBackend);
        CheckCritical(m_meshManager.Init(&m_renderBackend, &m_bufferManager),
                      "Failed to create the merged scene geometry buffers!");

        CheckCritical(m_globalSet.Init(rbi), "Failed to create the global resource set!");
        //! Sampler manager writes to global set during init
        CheckCritical(m_samplerManager.Init(rbi, m_globalSet), "Failed to initialize the sampler manager!");

        CheckCritical(m_frameConstants.Init(m_bufferManager, "FrameConstantsRing"),
                      "Failed to create the frame constants ring!");
        CheckCritical(m_objectData.Init(m_bufferManager, "ObjectDataRing", Conf::MAX_SCENE_OBJECTS),
                      "Failed to create the object data ring!");

        RenderContext& tctx = m_renderBackend.GetTransferContext();
        RenderContextEncoder* tEncoder = tctx.CreateCommandEncoder();
        tctx.BeginCmds();
        tEncoder->PushDebugGroup("InitUploads", {0.85f, 0.55f, 0.20f, 1.0f});

        m_textureLoader = std::make_unique<StbLoader>();
        m_textureManager = std::make_unique<Graphics::TextureManager>(m_textureLoader.get(), &m_renderBackend, &m_globalSet, tctx.CreateCommandEncoder());

        m_textureManager->GetOrLoadTexture(Shift::Util::GetShiftRoot() + "Assets/Textures/NB.jpg", tEncoder);
        CheckCritical(LoadScene(*tEncoder), "Failed to load the scene!");

        {
            PipelineDescriptor forwardDescriptor;
            forwardDescriptor.name = "ForwardOpaque";

            ShaderDescriptor vsDescriptor;
            vsDescriptor.type = EShaderType::Vertex;
            vsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Forward/ForwardVS.slang";
            vsDescriptor.entry = "mainVS";
            ShaderDescriptor fsDescriptor;
            fsDescriptor.type = EShaderType::Fragment;
            fsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Forward/ForwardPS.slang";
            fsDescriptor.entry = "mainPS";

            forwardDescriptor.colorBlendConfig.attachments.push_back({.format = VIEWPORT_COLOR_FORMAT});

            forwardDescriptor.depthStencilConfig = {
                .depthFormat = VIEWPORT_DEPTH_FORMAT,
                .depthTestEnabled = true,
                .depthWriteEnabled = true,
                .depthFunction = ECompareOperation::Less
            };

            forwardDescriptor.rasterizerStateDesc.cullMode = ECullMode::Back;
            forwardDescriptor.rasterizerStateDesc.windingOrder = EWindingOrder::CounterClockwise;

            forwardDescriptor.pushConstants = PushConstantRange{
                .offset = 0,
                .size = static_cast<uint32_t>(sizeof(GPU::PushConstants)),
                .stageFlags = EBindingVisibility::Vertex | EBindingVisibility::Fragment
            };

            forwardDescriptor.descriptorLayouts.resize(GlobalResourceSet::SET_INDEX + 1);
            forwardDescriptor.descriptorLayouts[GlobalResourceSet::SET_INDEX] = GlobalResourceSet::Layout();

            std::array<ShaderDescriptor, 2> shaderSources{vsDescriptor, fsDescriptor};
            m_forwardPipeline = m_pipelineManager.CreatePipeline(forwardDescriptor, shaderSources);
        }

        {
            m_viewportSamplerIdx = m_samplerManager.GetOrCreate(
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
                    .format = VIEWPORT_COLOR_FORMAT,
                    .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::Sampled,
                    .name = "ViewportRT"
                }
            );

            m_viewportDepth = CreateViewportDepthTexture(m_window.GetWidth(), m_window.GetHeight());
            CheckCritical(m_viewportDepth != nullptr, "Failed to create the viewport depth buffer!");
        }
        //! First frame the viewport may be hidden so we immediately transfer this

        tEncoder->PopDebugGroup();
        tctx.EndCmds();

        std::array sigPayloads{m_renderBackend.ReserveTransferSignalPayload()};
        CheckCritical(tctx.SubmitCmds({}, sigPayloads), "Failed to submit transition context!");

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
        m_meshManager.FreeStagingBuffers();

        return true;
    }

    Texture* Renderer::CreateViewportDepthTexture(uint32_t width, uint32_t height) {
        return m_renderBackend.CreateInterface()->CreateTexture(
            TextureDescriptor::CreateDepthTextureDesc(width, height, "ViewportDepth", VIEWPORT_DEPTH_FORMAT));
    }

    void Renderer::RegisterViewportTexture() {
        Sampler* viewportSampler = m_samplerManager.Get(m_viewportSamplerIdx);
        CheckCriticalEmptyReturn(viewportSampler != nullptr, "The viewport sampler index resolves to nothing!");

        m_viewportTextureID = ImGuiBackend::RegisterTexture(*viewportTexture, *viewportSampler);
    }

    bool Renderer::LoadScene(RenderContextEncoder& transferEncoder) {
        struct SceneModel {
            std::string path;
            glm::mat4 transform;
        };

        const std::string root = Shift::Util::GetShiftRoot();
        const std::vector<SceneModel> sources{
            {
                root + "Assets/Models/DamagedHelmet/scene.gltf",
                glm::scale(glm::translate(glm::mat4(1.0f), {-0.75f, 0.0f, -1.5f}), glm::vec3(0.3f))
            },
            {
                root + "Assets/Models/HumanSkull/scene.gltf",
                glm::scale(glm::translate(glm::mat4(1.0f), {0.75f, 0.0f, -1.5f}), glm::vec3(0.3f))
            },
        };

        GltfLoader loader;
        for (const SceneModel& source : sources) {
            std::optional<ModelData> model = loader.LoadFromFile(source.path);
            if (!model) {
                Log(Warning, "Scene model not loaded, skipping it: {}", source.path);
                continue;
            }

            //! Uniform scale, so the length of any basis vector of the transform IS the scale
            const float scale = glm::length(glm::vec3(source.transform[0]));

            for (const MeshData& meshData : model->meshes) {
                const MeshHandle handle = m_meshManager.UploadMesh(meshData, transferEncoder);
                if (!m_meshManager.IsValid(handle)) { continue; }
                const glm::vec3 centre = glm::vec3(source.transform * glm::vec4(glm::vec3(meshData.bounds.sphere), 1.0f));
                m_sceneInstances.push_back({
                    .mesh = handle,
                    .transform = source.transform,
                    .boundsSphere = glm::vec4(centre, meshData.bounds.sphere.w * scale)
                });
            }
        }
        CheckCritical(m_sceneInstances.size() <= Conf::MAX_SCENE_OBJECTS,
                      "The scene holds more objects than one ObjectData ring slot can carry!");

        Log(Info, "Scene loaded: {} instances, {} vertices and {} indices merged",
            m_sceneInstances.size(), m_meshManager.GetUsedVertices(), m_meshManager.GetUsedIndices());

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
        GPU::ObjectData* objectData = m_objectData.Slot(frameSlot);
        CheckCritical(objectData != nullptr, "Object data ring has no slot for this frame!");

        FillObjectData(objectData);
        FillFrameConstants(frameConstants, engineData, frameSlot);

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
            gEncoder->TransitionTexture(*m_viewportDepth, EResourceLayout::DepthStencilAttachmentOptimal, EPipelineStageFlags::EarlyFragmentTestsBit);

            RenderPassDescriptor renderPass;
            renderPass.colorAttachments.push_back(
                {
                    .renderTargetName = "ViewportTexture",
                    .clearValue = {.color = {0.3f, 0.3f, 0.3f, 1.0f}}
                }
            );
            renderPass.depthAttachment = RenderPassDescriptor::RenderPassAttachmentInfo{
                .renderTargetName = "ViewportDepth",
                .clearValue = {.depthStencil = {1.0f, 0u}}
            };
            renderPass.extent = {viewportTexture->GetWidth(), viewportTexture->GetHeight()};
            renderPass.enableSecondaryCommandBuffers = true;
            std::array colorTextures{viewportTexture};
            gEncoder->BeginRenderPass(renderPass, colorTextures, m_viewportDepth);
            // Reserve a single graphics signal payload and use it both for:
            // - telling the deferred executor when it's safe to free secondaries
            // - signalling from the primary submit
            auto graphicsSignal = m_renderBackend.ReserveGraphicsSignalPayload();

            // Acquire two secondary contexts (for current frame)
            RenderContext* sec0 = m_renderBackend.AcquireSecondaryGraphicsContext();
            RenderContext* sec1 = m_renderBackend.AcquireSecondaryGraphicsContext();

            Pipeline* pipeline = m_pipelineManager.Get(m_forwardPipeline);

            std::vector<ETextureFormat> colorTexturesFormats{};
            for (auto& c: pipeline->GetDescriptor().colorBlendConfig.attachments) {
                colorTexturesFormats.push_back(c.format);
            }

            SecondaryBufferBeginPayload payload{
                .colorFormats = colorTexturesFormats,
                .depthFormat = pipeline->GetDescriptor().depthStencilConfig.depthFormat
                // .stencilFormat = pipeline->GetDescriptor().depthStencilConfig.stencilFormat
            };

            //! temp
            constexpr uint32_t SECONDARY_COUNT = 2;
            const uint32_t instanceCount = static_cast<uint32_t>(m_sceneInstances.size());

            if (sec0 && sec1) {
                // Record secondaries on two threads.
                auto record_secondary = [&](RHIContext<RHI::Vulkan>* sec, uint32_t secIdx) {
                    // NOTE: AcquireSecondaryGraphicsContext already called ResetCmds() on the context.
                    CheckCritical(sec->BeginSecondaryCmds(payload), "Failed to begin secondary command buffer!");

                    Rect2D scissor = {{0, 0}, {viewportTexture->GetWidth(), viewportTexture->GetHeight()}};
                    Viewport viewport = {0.0f, static_cast<float>(viewportTexture->GetHeight()), static_cast<float>(viewportTexture->GetWidth()), -static_cast<float>(viewportTexture->GetHeight()), 0.0f, 1.0f};

                    RenderContextEncoder* secEncoder = sec->CreateCommandEncoder();

                    secEncoder->SetScissor(scissor);
                    secEncoder->SetViewport(viewport);

                    secEncoder->BindGraphicsPipeline(*pipeline);

                    secEncoder->BindResourceSet(*pipeline, GlobalResourceSet::SET_INDEX, *m_globalSet.Get());

                    Buffer* indexBuffer = m_meshManager.GetIndexBuffer();
                    CheckCritical(indexBuffer != nullptr, "The merged index buffer does not resolve!");
                    secEncoder->BindIndexBuffer({indexBuffer, 0}, EIndexSize::UInt32);

                    for (uint32_t i = secIdx; i < instanceCount; i += SECONDARY_COUNT) {
                        const Mesh* mesh = m_meshManager.Get(m_sceneInstances[i].mesh);
                        if (mesh == nullptr) { continue; }
                        const GPU::PushConstants push{
                            .frameConstantsRef = m_frameConstants.SlotAddress(frameSlot),
                            .objectIndex = i
                        };
                        secEncoder->SetPushConstants(*pipeline, &push, static_cast<uint32_t>(sizeof(push)));

                        for (const SubmeshDesc& submesh : mesh->submeshes) {
                            secEncoder->DrawIndexed({
                                .indexCount = submesh.indexCount,
                                .instanceCount = 1,
                                //! Mesh-local firstIndex plus where the mesh's indices landed
                                .firstIndex = mesh->indexRange.first + submesh.firstIndex,
                                .vertexOffset = 0,
                                .firstInstance = 0
                            });
                        }
                    }

                    CheckCritical(sec->EndCmds(), "Failed to end secondary command buffer!");
                    return true;
                };

                std::thread th0(record_secondary, sec0, 0u);
                std::thread th1(record_secondary, sec1, 1u);

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
        delete m_viewportDepth;

        m_meshManager.Destroy();
        m_bufferManager.Destroy();
        m_pipelineManager.Destroy();
        m_shaderManager.Destroy();

        m_textureManager.reset();
        m_textureLoader.reset();

        m_samplerManager.Destroy();
        m_globalSet.Destroy();

        m_renderBackend.Destroy();
    }

    void Renderer::ResizeViewport(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0) return;
        if (width == viewportTexture->GetWidth() && height == viewportTexture->GetHeight()) return;

        //! Follow the viewport and not full window
        m_controller->UpdateScreenSize(static_cast<float>(width), static_cast<float>(height));

        Texture* oldTexture = viewportTexture;
        Texture* oldDepth = m_viewportDepth;
        void* oldID = m_viewportTextureID;

        viewportTexture = m_renderBackend.CreateInterface()->CreateTexture({
            .width = width,
            .height = height,
            .format = VIEWPORT_COLOR_FORMAT,
            .usageFlags = ETextureUsageFlags::ColorAttachment | ETextureUsageFlags::Sampled,
            .name = "ViewportRT"
        });

        m_viewportDepth = CreateViewportDepthTexture(width, height);

        RegisterViewportTexture();

        //! Destruction deferred to current frame + MAX FIF because prev frames might have already submitted write commands to viewport before resize
        m_renderBackend.DeferExecuteToFrame(m_renderBackend.GetCurrentGlobalIndex() + Conf::SHIFT_MAX_FRAMES_IN_FLIGHT, [oldTexture, oldDepth, oldID]() mutable {
            if (oldID) {
                ImGuiBackend::UnregisterTexture(oldID);
            }
            delete oldTexture;
            delete oldDepth;
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

    void Renderer::FillFrameConstants(GPU::FrameConstants *frameConstantsPtr, const EngineData &engineData, uint32_t frameSlot) {
        frameConstantsPtr->cameraPosExposure = glm::vec4(engineData.camPosition, 1.0f);
        frameConstantsPtr->view = engineData.viewMatrix;
        frameConstantsPtr->proj = engineData.projMatrix;
        frameConstantsPtr->viewProj = engineData.projMatrix * engineData.viewMatrix;
        frameConstantsPtr->cameraDir = glm::vec4(engineData.camDirection, 0.0f);
        frameConstantsPtr->lightCount = 0u;
        frameConstantsPtr->selectedObject = UINT32_MAX;
        frameConstantsPtr->debugViewMode = 0u;

        frameConstantsPtr->positionsRef = m_meshManager.GetStreamAddress(MeshManager::Positions);
        frameConstantsPtr->normalsRef = m_meshManager.GetStreamAddress(MeshManager::Normals);
        frameConstantsPtr->tangentsRef = m_meshManager.GetStreamAddress(MeshManager::Tangents);
        frameConstantsPtr->uvsRef = m_meshManager.GetStreamAddress(MeshManager::UVs);

        frameConstantsPtr->objectBufferRef = m_objectData.SlotAddress(frameSlot);
        frameConstantsPtr->materialBufferRef = 0;
        frameConstantsPtr->lightBufferRef = 0;
    }

    void Renderer::FillObjectData(GPU::ObjectData *objectDataPtr) {
        const uint32_t count = std::min(static_cast<uint32_t>(m_sceneInstances.size()), m_objectData.GetElementsPerSlot());

        for (uint32_t i = 0; i < count; ++i) {
            const SceneInstance& instance = m_sceneInstances[i];
            const Mesh* mesh = m_meshManager.Get(instance.mesh);
            if (mesh == nullptr) { continue; }

            GPU::ObjectData& object = objectDataPtr[i];
            object.model = instance.transform;
            object.normalMat = glm::transpose(glm::inverse(instance.transform));
            object.boundsSphere = instance.boundsSphere;
            object.materialIndex = 0u;
            object.vertexOffset = mesh->vertexRange.first;
        }
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
