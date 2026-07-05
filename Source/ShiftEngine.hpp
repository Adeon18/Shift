//
// Created by otrush on 2/27/2024.
//

#ifndef SHIFT_SHIFTENGINE_HPP
#define SHIFT_SHIFTENGINE_HPP

#include <memory>
#include <optional>
#include <string>

#include <glm/glm.hpp>

#include "Graphics/Renderer.hpp"

#include "Window/ShiftWindow.hpp"
#include "Tools/Timer/FPSTimer.hpp"
#include "Graphics/UI/EditorLayer.hpp"

namespace Shift {
    //! Creation-time engine configuration. Defaults boot the standard windowed editor
    //! tests override (hidden window, forced backend features such as sync-validation)
    struct EngineDescriptor {
        uint32_t width = 1080;
        uint32_t height = 720;
        std::string windowName = "Shift";
        bool windowVisible = true;
        //! When set, replaces the backend's default RHIRequiredFeatures for this run
        //! Used in tests, mostly
        std::optional<RHIRequiredFeatures> featuresOverride{};
    };

    class ShiftEngine {
    public:
        ShiftEngine(): m_timer{200.0f} {}

        //! Initialize the Engine, initializes all internal components, which helps con control failure at startup
        bool Init(const EngineDescriptor& desc = {});
        //! Load the scene data
        bool LoadScene(std::string filepath);
        //! Engine loop
        bool Run();
        //! One full engine frame driven by an injected dt: events, input, camera, render, input
        //! rollover. Run() feeds it real frame time, tests feed a fixed dt for determinism
        bool Tick(float dt);
        //! Call clean on all the resources
        void Cleanup();

        //! Subobject access, valid only after a successful Init
        //! Here because tests need finer control
        [[nodiscard]] Graphics::Renderer& GetRenderer() { return *m_renderer; }
        [[nodiscard]] ShiftWindow& GetWindow() { return *m_window; }

    private:
        void FillEngineData(float dt);
        void HandleInput();

        Graphics::EngineData m_engineData;

        std::unique_ptr<Graphics::Renderer> m_renderer;

        std::unique_ptr<ShiftWindow> m_window;

        std::shared_ptr<ctrl::FlyingCameraController> m_controller;
        tool::FPSTimer m_timer;

        //! Sum of ticked dt
        float m_timeSinceStart = 0.0f;

        Editor::EditorLayer m_editorLayer;
    };
} // shift


#endif //SHIFT_SHIFTENGINE_HPP
