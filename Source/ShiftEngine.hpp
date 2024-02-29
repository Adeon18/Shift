//
// Created by otrush on 2/27/2024.
//

#ifndef SHIFT_SHIFTENGINE_HPP
#define SHIFT_SHIFTENGINE_HPP

#include <memory>

#include <glm/glm.hpp>

#include "Graphics/Renderer.hpp"

#include "Window/ShiftWindow.hpp"
#include "Tools/Timer/FPSTimer.hpp"

namespace shift {
    class ShiftEngine {
    public:
        ShiftEngine(): m_timer{300.0f} {}

        //! Initialize the Engine, initializes all internal components, which helps con control failure at startup
        bool Init(uint32_t width, uint32_t height);
        //! Load the scene data
        bool LoadScene(std::string filepath);
        //! Engine loop
        bool Run();
        //! Call clean on all the resources
        void Cleanup();

    private:
        void FillEngineData();
        void HandleInput();

        gfx::EngineData m_engineData;

        std::unique_ptr<gfx::Renderer> m_renderer;

        std::unique_ptr<ShiftWindow> m_window;

        ctrl::FlyingCameraController m_controller;
        tool::FPSTimer m_timer;
    };
} // shift


#endif //SHIFT_SHIFTENGINE_HPP
