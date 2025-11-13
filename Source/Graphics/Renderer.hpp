//
// Created by otrush on 2/27/2024.
//

#ifndef SHIFT_RENDERER_HPP
#define SHIFT_RENDERER_HPP

#include <glm/glm.hpp>

#include "Window/ShiftWindow.hpp"
#include "Input/Controllers/Camera/FlyingCameraController.hpp"

#include "Graphics/RHI/RHI.hpp"

namespace Shift::gfx {
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
        // class UI: public ui::UIWindowComponent {
        // public:
        //     explicit UI(std::string name, std::string sName, Renderer& system): ui::UIWindowComponent{std::move(name), std::move(sName)}, m_system{system} {}
        //
        //     virtual void Item() override { ui::UIWindowComponent::Item(); }
        //     virtual void Show(uint32_t currentFrame) override;
        //
        //     SGUID dummy;
        //     glm::vec4 rotation{0.0f, 1.0f, 0.0f, 0.0f};
        //     float rotSpeedIncrementTimes1k = 3.f;
        // private:
        //     Renderer& m_system;
        // };
    public:
        Renderer(ShiftWindow& window, std::shared_ptr<ctrl::FlyingCameraController> controller): m_window{window}, m_controller(controller) {

        }

        //! Init the renderer and all child elements
        bool Init();

        // TODO: hardcoded
        bool LoadScene();

        //! Render entire frame
        bool RenderFrame(const EngineData& engineData);

        //! Cleanup unused resources
        void Cleanup();
    private:
        [[nodiscard]] uint32_t AquireImage(bool *success);
        [[nodiscard]] bool PresentFinalImage(uint32_t imageIndex);

        ShiftWindow& m_window;
        std::shared_ptr<ctrl::FlyingCameraController> m_controller;

        Pipeline p;
        Shader vs;
        Shader ps;
        Buffer vertex;

        SRHI m_SRHI;

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
