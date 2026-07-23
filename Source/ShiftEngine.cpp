//
// Created by otrush on 2/27/2024.
//

#include "ShiftEngine.hpp"

#include "spdlog/spdlog.h"

#include "Utility/FileWatcher/FileWatcher.hpp"

#include "Graphics/UI/ViewportPanel.hpp"
#include "Graphics/UI/GenericPanel.hpp"

namespace Shift {
    bool ShiftEngine::Init(const EngineDescriptor& desc) {
        spdlog::set_level(spdlog::level::trace);

        Util::FileWatcher::Get().Init();
        m_window = std::make_unique<Shift::ShiftWindow>(desc.width, desc.height, desc.windowName, desc.windowVisible);

        const glm::vec3 pos = glm::vec3(0.0f, 0.0f, 1.0f);
        std::pair<uint32_t, uint32_t> sizes{m_window->GetWidth(), m_window->GetHeight()};
        m_controller = std::make_shared<ctrl::FlyingCameraController>(80.0f, sizes, pos);

        m_renderer = std::make_unique<Graphics::Renderer>(*m_window, m_controller);
        if (!m_renderer->Init(desc.featuresOverride)) { return false;}

        m_editorLayer.Init(*m_window, m_renderer->GetRHILocal<ShiftSelectedAPI>());

        auto& ctx = m_editorLayer.GetContext();

        ctx.OnViewportResize = [this](uint32_t w, uint32_t h) {
            m_renderer->ResizeViewport(w, h);
        };

        m_editorLayer.AddPanel<Editor::ViewportPanel>(m_editorLayer.GetContext());
        m_editorLayer.AddPanel<Editor::GenericPanel>("Properties");
        m_editorLayer.AddPanel<Editor::GenericPanel>("Scene Hierarchy");
        m_editorLayer.AddPanel<Editor::GenericPanel>("Content Browser");
        m_editorLayer.AddPanel<Editor::GenericPanel>("Console");

        m_renderer->RegisterViewportTexture();

        return true;
    }

    bool ShiftEngine::LoadScene(std::string filepath) {
        return true;
    }

    bool ShiftEngine::Run() {
        while (m_window->IsActive()) {
            if (m_timer.HasFrameElapsed()) {
                auto showFPS = m_timer.IsDebugFPSShow();
                if (showFPS.first) {
                    spdlog::debug("Shift FPS: {}", showFPS.second);
                    //! GPU timing breakdown of the last completed frame
                    for (const auto& range : m_renderer->GetLastFrameGPUTimeRanges()) {
                        spdlog::debug("  GPU {}{}: {:.3f} ms", std::string(range.depth * 2, ' '), range.name, range.milliseconds);
                    }
                }

                if (!Tick(m_timer.GetDt())) {
                    return false;
                }
            }
        }

        return true;
    }

    bool ShiftEngine::Tick(float dt) {
        m_timeSinceStart += dt;

        m_window->Process();

        Util::FileWatcher::Get().Poll();

        HandleInput();
        m_controller->CaptureInputAndApply(dt);

        FillEngineData(dt);
        if (!m_renderer->RenderFrame(m_engineData, &m_editorLayer)) {
            return false;
        }
        inp::Keyboard::GetInstance().UpdateKeys();
        inp::Mouse::GetInstance().UpdatePos();

        return true;
    }

    void ShiftEngine::Cleanup() {
        m_renderer->WaitForCleanup();
        m_editorLayer.Destroy();
        m_renderer->Cleanup();
        m_window.reset();
    }

    void ShiftEngine::FillEngineData(float dt) {
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

        //! All timing derives from the injected dt so a fixed-dt run is fully deterministic
        m_engineData.dt = dt;
        m_engineData.fps = (dt > 0.0f) ? (1.0f / dt) : 0.0f;
        m_engineData.secondsSinceStart = m_timeSinceStart;
        m_engineData.frameTimeMs = dt * 1000.0f;
    }

    void ShiftEngine::HandleInput() {
        if (inp::Mouse::GetInstance().isRightButtonPressed()) {
            m_window->SetCaptureCursor(true);
        } else {
            m_window->SetCaptureCursor(false);
        }

        if (inp::Keyboard::GetInstance().IsJustPressed(GLFW_KEY_R)) {
            m_renderer->HotReloadShaders();
        }
    }
} // shift
