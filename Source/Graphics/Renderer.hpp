//
// Created by otrush on 2/27/2024.
//

#ifndef SHIFT_RENDERER_HPP
#define SHIFT_RENDERER_HPP

#include <glm/glm.hpp>

#include "Window/ShiftWindow.hpp"
#include "Input/Controllers/Camera/FlyingCameraController.hpp"

#include "Graphics/RHI/RHI.hpp"
//! TODO [Design]Editor leak to renderer
#include "Graphics/UI/EditorLayer.hpp"

#include "Graphics/Managers/TextureManager.hpp"
#include "Graphics/Managers/ShaderManager.hpp"
#include "Graphics/Managers/PipelineManager.hpp"
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
        Renderer(ShiftWindow& window, std::shared_ptr<ctrl::FlyingCameraController> controller): m_window{window}, m_controller(controller) {

        }

        //! Init the renderer and all child elements
        bool Init();

        void RegisterViewportTexture();

        // TODO: hardcoded
        bool LoadScene();

        //! Render entire frame
        bool RenderFrame(const EngineData& engineData, Editor::EditorLayer* editor);

        void HotReloadShaders();

        //! TODO [Design] is this bad?
        void WaitForCleanup();

        void ResizeViewport(uint32_t width, uint32_t height);

        //! Cleanup unused resources
        void Cleanup();

        [[nodiscard]] void* GetViewportTextureID() const { return m_viewportTextureID; }
        template<typename API>
        [[nodiscard]] const RHILocal<API>& GetRHILocal() const { return m_renderBackend.GetLocal();}

    private:
        [[nodiscard]] uint32_t AquireImage(bool *success);
        [[nodiscard]] bool PresentFinalImage(uint32_t imageIndex);

        ShiftWindow& m_window;
        std::shared_ptr<ctrl::FlyingCameraController> m_controller;

        //! Opaque handle into m_pipelineManager (which owns the pipeline); resolved to a Pipeline&
        //! via m_pipelineManager.Get() at the bind site.
        Graphics::PipelineHandle m_pipeline;
        Buffer* vertex;

        RenderBackend m_renderBackend;

        //! Asset/pipeline managers live above the RHI and are owned here
        Graphics::ShaderManager m_shaderManager;
        Graphics::PipelineManager m_pipelineManager;

        Texture* viewportTexture;
        Sampler* viewportSampler;
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
