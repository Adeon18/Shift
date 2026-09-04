//
// Created by otrush on 2/27/2024.
//

#ifndef SHIFT_RENDERER_HPP
#define SHIFT_RENDERER_HPP

#include <optional>

#include <glm/glm.hpp>

#include "Window/ShiftWindow.hpp"
#include "Input/Controllers/Camera/FlyingCameraController.hpp"

#include "Graphics/RHI/RHI.hpp"
//! TODO [Design]Editor leak to renderer
#include "Graphics/UI/EditorLayer.hpp"

#include "Graphics/Managers/GlobalResourceSet.hpp"
#include "Graphics/Managers/SamplerManager.hpp"
#include "Graphics/Managers/TextureManager.hpp"
#include "Graphics/Managers/ShaderManager.hpp"
#include "Graphics/Managers/PipelineManager.hpp"
#include "Graphics/Managers/BufferManager.hpp"
#include "Graphics/Managers/MeshManager.hpp"
#include "Graphics/Managers/MaterialManager.hpp"
#include "Graphics/Managers/FrameRingBuffer.hpp"
#include "Graphics/RenderScene.hpp"
#include "Graphics/Shared/GPUShared.h"
#include "Loaders/TextureLoader/StbLoader.hpp"

namespace Shift::Graphics {
    //! A struct with data that can change per-frame
    struct EngineData {
        /// Camera controller
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::vec3 camPosition;
        glm::vec3 camDirection;
        glm::vec3 camRight;
        glm::vec3 camUp;
        /// Window data
        uint32_t winWidth;
        uint32_t winHeight;
        float oneDivWinWidth;
        float oneDivWinHeight;
        /// Timer data
        float dt;
        float fps;
        float secondsSinceStart;
        float frameTimeMs;
    };

    class Renderer {

    public:
        static constexpr ETextureFormat VIEWPORT_COLOR_FORMAT = ETextureFormat::B8G8R8A8_SRGB;
        static constexpr ETextureFormat VIEWPORT_DEPTH_FORMAT = ETextureFormat::D32_SFLOAT;

        Renderer(ShiftWindow& window, std::shared_ptr<ctrl::FlyingCameraController> controller): m_window{window}, m_controller(controller) {

        }

        //! Init the renderer and all child elements. featuresOverride replaces the backend's
        //! default RHIRequiredFeatures for this run
        bool Init(const std::optional<RHIRequiredFeatures>& featuresOverride = std::nullopt);

        void RegisterViewportTexture();

        // TODO: hardcoded until the Scene container
        bool LoadScene(RenderContextEncoder& transferEncoder);

        //! Render entire frame
        bool RenderFrame(const EngineData& engineData, Editor::EditorLayer* editor);

        //! Rebuild every pipeline whose shaders are marked dirty, and returns the rebuild pipeline count
        uint32_t HotReloadShaders();

        //! TODO [Design] is this bad?
        void WaitForCleanup();

        void ResizeViewport(uint32_t width, uint32_t height);

        //! Cleanup unused resources
        void Cleanup();

        [[nodiscard]] void* GetViewportTextureID() const { return m_viewportTextureID; }
        template<typename API>
        [[nodiscard]] const RHILocal<API>& GetRHILocal() const { return m_renderBackend.GetLocal();}

        [[nodiscard]] ShaderManager& GetShaderManager() { return m_shaderManager; }

        [[nodiscard]] PipelineManager& GetPipelineManager() { return m_pipelineManager; }

        [[nodiscard]] TextureManager& GetTextureManager() { return *m_textureManager; }

        [[nodiscard]] SamplerManager& GetSamplerManager() { return m_samplerManager; }

        [[nodiscard]] GlobalResourceSet& GetGlobalResourceSet() { return m_globalSet; }

        [[nodiscard]] MeshManager& GetMeshManager() { return m_meshManager; }

        [[nodiscard]] MaterialManager& GetMaterialManager() { return m_materialManager; }

        //! The instabnces the scene pass draws, in ObjectData order
        [[nodiscard]] const std::vector<MeshPlacement>& GetPlacements() const { return m_placements; }

        //! object array + draw list
        [[nodiscard]] const RenderScene& GetRenderScene() const { return m_renderScene; }

        //! The frame constants of one in-flight slot
        [[nodiscard]] const GPU::FrameConstants* GetFrameConstants(uint32_t frameSlot) const {
            return m_frameConstants.Slot(frameSlot);
        }

        [[nodiscard]] const GPU::ObjectData* GetObjectData(uint32_t frameSlot) const {
            return m_objectData.Slot(frameSlot);
        }

        [[nodiscard]] const GPU::MaterialData* GetMaterialData(uint32_t frameSlot) const {
            return m_materialData.Slot(frameSlot);
        }

        //! Resolved GPU timing ranges of the most recently completed frame
        [[nodiscard]] const std::vector<GPUTimeRange>& GetLastFrameGPUTimeRanges() const { return m_renderBackend.GetLastFrameGPUTimeRanges(); }

    private:
        //! Builds a callable function per mesh to be used by mat manager to map texture to bindless index -> here temporarily until scene class
        [[nodiscard]] TextureSlotResolver MakeTextureResolver(const std::string& baseDir,
                                                              RenderContextEncoder& transferEncoder);

        [[nodiscard]] uint32_t AquireImage(bool *success);
        [[nodiscard]] bool PresentFinalImage(uint32_t imageIndex);
        void FillFrameConstants(GPU::FrameConstants* frameConstantsPtr, const EngineData& engineData, uint32_t frameSlot);

        void UploadObjectData(GPU::ObjectData* objectDataPtr);

        void UploadMaterialData(GPU::MaterialData* materialDataPtr);

        [[nodiscard]] bool RecordDrawItems(RenderContextEncoder& encoder, uint32_t frameSlot);

        [[nodiscard]] Texture* CreateViewportDepthTexture(uint32_t width, uint32_t height);

        ShiftWindow& m_window;
        std::shared_ptr<ctrl::FlyingCameraController> m_controller;

        Graphics::PipelineHandle m_forwardPipeline;

        RenderBackend m_renderBackend;

        //! Asset/pipeline managers live above the RHI and are owned here
        Graphics::ShaderManager m_shaderManager;
        Graphics::PipelineManager m_pipelineManager;
        Graphics::BufferManager m_bufferManager;
        Graphics::MeshManager m_meshManager;
        Graphics::MaterialManager m_materialManager;
        Graphics::SamplerManager m_samplerManager;

        //! Bindless set b0
        Graphics::GlobalResourceSet m_globalSet;

        //! One FrameConstants slot per frame in flight
        Graphics::FrameRingBuffer<GPU::FrameConstants> m_frameConstants;
        //! One ObjectData array per FIF. Updated every frame
        Graphics::FrameRingBuffer<GPU::ObjectData> m_objectData;
        //! One MaterialData array per FIF. Pulled from MaterialManager every frame
        Graphics::FrameRingBuffer<GPU::MaterialData> m_materialData;

        std::vector<MeshPlacement> m_placements;
        //! Thr renderer scvene input
        RenderScene m_renderScene;

        Texture* viewportTexture = nullptr;
        Texture* m_viewportDepth = nullptr;
        //! Slot in the global sampler array
        uint32_t m_viewportSamplerIdx = 0;
        void* m_viewportTextureID = nullptr;

        std::unique_ptr<ITextureLoader> m_textureLoader;
        std::unique_ptr<Graphics::TextureManager> m_textureManager;

        //! Shift API
        // ShiftBackBuffer m_backBuffer;
        // ShiftContext m_context;
        // std::unique_ptr<ModelManager> m_modelManager;
        // std::unique_ptr<DescriptorManager> m_descriptorManager;
        // std::unique_ptr<BufferManager> m_bufferManager;
        // std::unique_ptr<SamplerManager> m_samplerManager;
        // std::unique_ptr<RenderTargetManager> m_RTManager;
        // std::unique_ptr<TextureManager> m_textureManager;

        //! Shift Rendering Systems
        // std::unique_ptr<GeometrySystem> m_meshSystem;
        // std::unique_ptr<PostProcessSystem> m_postProcessSystem;
        // std::unique_ptr<LightSystem> m_lightSystem;
        // std::unique_ptr<ProfilingSystem> m_profilingSystem;

        // UI m_debugUI{"Master Renderer", "Debug", *this};
    };
} // shift::gfx


#endif //SHIFT_RENDERER_HPP
