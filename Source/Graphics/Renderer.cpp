//
// Created by otrush on 2/27/2024.
//

#include "Renderer.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"

#include <algorithm>
#include <cstring>

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
        CheckCritical(m_materialData.Init(m_bufferManager, "MaterialDataRing", Conf::MAX_SCENE_MATERIALS),
                      "Failed to create the material data ring!");

        RenderContext& tctx = m_renderBackend.GetTransferContext();
        RenderContextEncoder* tEncoder = tctx.CreateCommandEncoder();
        tctx.BeginCmds();
        tEncoder->PushDebugGroup("InitUploads", {0.85f, 0.55f, 0.20f, 1.0f});

        m_textureLoader = std::make_unique<StbLoader>();
        m_textureManager = std::make_unique<Graphics::TextureManager>(m_textureLoader.get(), &m_renderBackend, &m_globalSet, tctx.CreateCommandEncoder());

        m_textureManager->GetOrLoadTexture(Shift::Util::GetShiftRoot() + "Assets/Textures/NB.jpg", tEncoder);

        CheckCritical(m_materialManager.Init(MakeTextureResolver("", *tEncoder)),
                      "Failed to initialize the material manager!");

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
                glm::scale(glm::translate(glm::mat4(1.0f), {-2.0f, 0.0f, -3.5f}), glm::vec3(1.0))
            },
            {
                root + "Assets/Models/HumanSkull/scene.gltf",
                glm::scale(glm::translate(glm::mat4(1.0f), {2.0f, 0.0f, -3.5f}), glm::vec3(1.0f))
            },
        };

        GltfLoader loader;
        for (const SceneModel& source : sources) {
            std::optional<ModelData> model = loader.LoadFromFile(source.path);
            if (!model) {
                Log(Warning, "Scene model not loaded, skipping it: {}", source.path);
                continue;
            }

            //! Get the global remap and make ressolver to index
            const std::vector<uint32_t> materialRemap = m_materialManager.RegisterModelMaterials(
                model->materials, MakeTextureResolver(Shift::Util::GetDirectoryFromPath(model->sourcePath), transferEncoder));

            //! Upload every mesh once
            std::vector<MeshHandle> meshHandles;
            meshHandles.reserve(model->meshes.size());
            for (const MeshData& meshData : model->meshes) {
                meshHandles.push_back(m_meshManager.UploadMesh(meshData, transferEncoder, materialRemap));
            }

            std::vector<glm::mat4> nodeWorld(model->nodes.size(), glm::mat4(1.0f));
            const size_t placedBefore = m_placements.size();
            //! Resolve  the evil ass node logic and "place" a mesh.
            for (size_t i = 0; i < model->nodes.size(); ++i) {
                const NodeDesc& node = model->nodes[i];
                const glm::mat4 local = glm::translate(glm::mat4(1.0f), node.translation)
                                      * glm::mat4_cast(node.rotation)
                                      * glm::scale(glm::mat4(1.0f), node.scale);
                nodeWorld[i] = (node.parent == MODEL_INDEX_NONE) ? local : nodeWorld[node.parent] * local;

                if (node.meshIndex == MODEL_INDEX_NONE) { continue; }
                if (node.meshIndex >= meshHandles.size()) { continue; }
                if (!m_meshManager.IsValid(meshHandles[node.meshIndex])) { continue; }

                m_placements.push_back({
                    .mesh = meshHandles[node.meshIndex],
                    .transform = source.transform * nodeWorld[i]
                });
            }

            //! Fallback if no node tree
            if (m_placements.size() == placedBefore) {
                Log(Warning, "Model '{}' has no nodes referencing its {} meshes; placing each at the "
                             "model transform", source.path, model->meshes.size());
                for (const MeshHandle handle : meshHandles) {
                    if (!m_meshManager.IsValid(handle)) { continue; }
                    m_placements.push_back({.mesh = handle, .transform = source.transform});
                }
            }
        }
        CheckCritical(m_placements.size() <= Conf::MAX_SCENE_OBJECTS,
                      "The scene holds more objects than one ObjectData ring slot can carry!");

        Log(Info, "Scene loaded: {} placements, {} materials, {} vertices and {} indices merged",
            m_placements.size(), m_materialManager.GetCount(),
            m_meshManager.GetUsedVertices(), m_meshManager.GetUsedIndices());

        return true;
    }

    TextureSlotResolver Renderer::MakeTextureResolver(const std::string& baseDir,
                                                      RenderContextEncoder& transferEncoder) {
        const uint32_t placeholderSlot = m_textureManager->GetPlaceholderHandle().slotIdx;
        return [this, baseDir, placeholderSlot, encoder = &transferEncoder](const TextureRef& ref) {
            //! THis is what gets triggered if there is no ORM texture in mesh for example
            if (!ref.IsSet()) { return placeholderSlot; }
            TextureHandle tex = m_textureManager->GetOrLoadTexture(baseDir + ref.id, encoder, ref.colorSpace);
            return tex.slotIdx;
        };
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

        //! Rebuld the draw list + objecty arr, culling, selection movement n shi will be here later
        m_renderScene.Extract(m_placements, m_meshManager, m_forwardPipeline);

        //! Reclaim this frame slot
        m_renderBackend.BeginFrame();

        const uint32_t frameSlot = m_renderBackend.GetCurrentFrame();
        GPU::FrameConstants* frameConstants = m_frameConstants.Slot(frameSlot);
        CheckCritical(frameConstants != nullptr, "Frame constants ring has no slot for this frame!");
        GPU::ObjectData* objectData = m_objectData.Slot(frameSlot);
        CheckCritical(objectData != nullptr, "Object data ring has no slot for this frame!");
        GPU::MaterialData* materialData = m_materialData.Slot(frameSlot);
        CheckCritical(materialData != nullptr, "Material data ring has no slot for this frame!");

        UploadObjectData(objectData);
        UploadMaterialData(materialData);
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
            std::array colorTextures{viewportTexture};
            gEncoder->BeginRenderPass(renderPass, colorTextures, m_viewportDepth);

            const Rect2D scissor = {{0, 0}, {viewportTexture->GetWidth(), viewportTexture->GetHeight()}};
            const Viewport viewport = {0.0f, static_cast<float>(viewportTexture->GetHeight()),
                                       static_cast<float>(viewportTexture->GetWidth()),
                                       -static_cast<float>(viewportTexture->GetHeight()), 0.0f, 1.0f};
            gEncoder->SetScissor(scissor);
            gEncoder->SetViewport(viewport);

            //! Bind global index buffer
            Buffer* indexBuffer = m_meshManager.GetIndexBuffer();
            CheckCritical(indexBuffer != nullptr, "The merged index buffer does not resolve!");
            gEncoder->BindIndexBuffer({indexBuffer, 0}, EIndexSize::UInt32);

            CheckCritical(RecordDrawItems(*gEncoder, frameSlot), "Failed to record the draw list!");

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
        frameConstantsPtr->materialBufferRef = m_materialData.SlotAddress(frameSlot);
        frameConstantsPtr->lightBufferRef = 0;
    }

    void Renderer::UploadObjectData(GPU::ObjectData *objectDataPtr) {
        const std::vector<GPU::ObjectData>& objects = m_renderScene.GetObjects();
        const uint32_t count = std::min(static_cast<uint32_t>(objects.size()), m_objectData.GetElementsPerSlot());
        if (count == 0) { return; }
        std::memcpy(objectDataPtr, objects.data(), static_cast<size_t>(count) * sizeof(GPU::ObjectData));
    }

    //! Same as above - prob should make a template
    void Renderer::UploadMaterialData(GPU::MaterialData *materialDataPtr) {
        const std::vector<GPU::MaterialData>& materials = m_materialManager.GetMaterials();
        const uint32_t count = std::min(static_cast<uint32_t>(materials.size()), m_materialData.GetElementsPerSlot());
        if (count == 0) { return; }
        std::memcpy(materialDataPtr, materials.data(), static_cast<size_t>(count) * sizeof(GPU::MaterialData));
    }

    bool Renderer::RecordDrawItems(RenderContextEncoder& encoder, uint32_t frameSlot) {
        const std::vector<DrawItem>& items = m_renderScene.GetDrawItems();
        const uint64_t frameConstantsRef = m_frameConstants.SlotAddress(frameSlot);

        PipelineHandle boundHandle{};
        Pipeline* boundPipeline = nullptr;
        bool setBound = false;

        for (const DrawItem& item : items) {
            if (!HasPass(item.passMask, EPassBit::Forward)) { continue; }

            //! Pipeline handling
            if (boundPipeline == nullptr || !(item.pipeline == boundHandle)) {
                Pipeline* next = m_pipelineManager.Get(item.pipeline);
                if (next == nullptr) { continue; }

                boundPipeline = next;
                boundHandle = item.pipeline;
                encoder.BindGraphicsPipeline(*boundPipeline);

                if (!setBound) {
                    encoder.BindResourceSet(*boundPipeline, GlobalResourceSet::SET_INDEX, *m_globalSet.Get());
                    setBound = true;
                }
            }

            //! Push constants
            const GPU::PushConstants push{
                .frameConstantsRef = frameConstantsRef,
                .objectIndex = item.objectIndex,
                .materialIndex = item.materialIndex,
                .meshVertexBase = item.meshVertexBase
            };
            encoder.SetPushConstants(*boundPipeline, &push, static_cast<uint32_t>(sizeof(push)));

            encoder.DrawIndexed({
                .indexCount = item.indexCount,
                .instanceCount = item.instanceCount,
                .firstIndex = item.firstIndex,
                .vertexOffset = 0,
                .firstInstance = 0
            });
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
