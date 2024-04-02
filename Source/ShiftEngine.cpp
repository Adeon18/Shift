//
// Created by otrush on 2/27/2024.
//

#include "ShiftEngine.hpp"

#include "spdlog/spdlog.h"

namespace shift {
    bool ShiftEngine::Init(uint32_t width, uint32_t height) {
        spdlog::set_level(spdlog::level::debug);

        m_window = std::make_unique<shift::ShiftWindow>(width, height, "Shift");

        const glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
        std::pair<uint32_t, uint32_t> sizes{m_window->GetWidth(), m_window->GetHeight()};
        m_controller = std::make_shared<ctrl::FlyingCameraController>(80.0f, sizes, pos);

        m_renderer = std::make_unique<gfx::Renderer>(*m_window, m_controller);
        if (!m_renderer->Init()) { return false;}

        return true;
    }

    bool ShiftEngine::LoadScene(std::string filepath) {
        return true;
    }

    bool ShiftEngine::Run() {
        while (m_window->IsActive()) {
            if (m_timer.HasFrameElapsed()) {
                m_window->Process();

                HandleInput();
                m_controller->CaptureInputAndApply(m_timer.GetDt());

                FillEngineData();
                if (!m_renderer->RenderFrame(m_engineData)) {
                    return false;
                }
                inp::Keyboard::GetInstance().UpdateKeys();
                inp::Mouse::GetInstance().UpdatePos();
            }
        }

        return true;
    }

    void ShiftEngine::Cleanup() {
        m_renderer->Cleanup();
        m_window.reset();
    }

    void ShiftEngine::FillEngineData() {
        m_engineData.viewMatrix = m_controller->GetCamera().GetViewMatrix();
        m_engineData.projMatrix = m_controller->GetCamera().GetProjectionMatrix();
        m_engineData.camDirection = m_controller->GetDirection();
        m_engineData.camPosition = m_controller->GetPosition();
        m_engineData.camRight = m_controller->GetRightDir();
        m_engineData.camUp = m_controller->GetUpDir();

        m_engineData.winWidth = m_window->GetWidth();
        m_engineData.winHeight = m_window->GetHeight();
        m_engineData.oneDivWinWidth = 1.0f / static_cast<float>(m_window->GetWidth());
        m_engineData.oneDivWinHeight =  1.0f / static_cast<float>(m_window->GetHeight());

        m_engineData.dt = m_timer.GetDt();
        m_engineData.fps = m_timer.GetFPSCurrent();
        m_engineData.secondsSinceStart = m_timer.GetSecondsSinceStart();
    }

    void ShiftEngine::HandleInput() {
        auto showFPS = m_timer.IsDebugFPSShow();
        if (showFPS.first) {
            spdlog::debug("Shift FPS: {}", showFPS.second);
        }

        if (inp::Mouse::GetInstance().isRightButtonPressed()) {
            m_window->SetCaptureCursor(true);
        } else {
            m_window->SetCaptureCursor(false);
        }
    }
} // shift

