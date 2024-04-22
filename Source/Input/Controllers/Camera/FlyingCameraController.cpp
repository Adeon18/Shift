#include "FlyingCameraController.hpp"
#include <glm/gtx/string_cast.hpp>

#include <iostream>

namespace shift::ctrl {
    FlyingCameraController::FlyingCameraController(float fovDeg,
                                                   std::pair<uint32_t, uint32_t> screenWH,
                                                   const glm::vec3 &position)
            : m_camera{fovDeg, screenWH.first, screenWH.second, position} {}

    void FlyingCameraController::CaptureInputAndApply(float dt) {
        if (inp::Keyboard::GetInstance().IsPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            m_movementSpeed += static_cast<float>(inp::Mouse::GetInstance().GetYScroll()) * m_movementSpeedChange;
            m_movementSpeed = glm::clamp(m_movementSpeed, m_movementSpeedMin, m_movementSpeedMax);
            HandleRotation(dt);
            HandleMovement(dt);
        }
    }

    void FlyingCameraController::UpdateScreenSize(float screenWidth,
                                                  float screenHeight) {
        m_camera.UpdateScreenSize(screenWidth, screenHeight);
    }

    void FlyingCameraController::MoveCamera(const glm::vec3 &offset) {
        m_camera.AddRelativeOffset(offset);
    }

    void FlyingCameraController::HandleRotation(float dt) {

        glm::vec3 rotation{0.0f, 0.0f, 0.0f};

        rotation.x -= static_cast<float>(inp::Mouse::GetInstance().GetYMovement()) * ROTATION_SPEED * m_camera.GetScreenRatio();
        rotation.y -= static_cast<float>(inp::Mouse::GetInstance().GetXMovement()) * ROTATION_SPEED * m_camera.GetScreenRatio();

        if (glm::length(rotation) > 0) {
            m_camera.AddRotation(rotation * dt);
        }
    }

    void FlyingCameraController::HandleMovement(float dt) {
        glm::vec3 direction{0.0f, 0.0f, 0.0f};

        for (auto& [k, v]: MOVEMENT_BIND_MAP) {
            if (inp::Keyboard::GetInstance().IsPressed(k)) {
                direction += v;
            }
        }
        if (glm::length(direction) > 0) {
            m_camera.AddRelativeOffset(glm::normalize(direction) * m_movementSpeed * dt);
        }
    }

    glm::vec3 FlyingCameraController::GetPosition() {
        return m_camera.GetPosition();
    }

    glm::vec3 FlyingCameraController::GetDirection() {
        return m_camera.GetFrontDirection();
    }

    glm::vec3 FlyingCameraController::GetRightDir() {
        return m_camera.GetRightDirection();
    }

    glm::vec3 FlyingCameraController::GetUpDir() {
        return m_camera.GetUpDirection();
    }

    void FlyingCameraController::UI::Show(uint32_t currentFrame) {
        if (m_shown) {
            ImGui::Begin(m_name.c_str(), &m_shown);

            ImGui::SeparatorText("Speed Settings");
            ImGui::DragFloat("Camera Speed", &m_controller.m_movementSpeed, 0.25f, m_controller.m_movementSpeedMin, m_controller.m_movementSpeedMax);
            ImGui::DragFloat("Camera Speed Min Limit", &m_controller.m_movementSpeedMin, 0.25f, 0.001f, m_controller.m_movementSpeedMax);
            ImGui::DragFloat("Camera Speed Max Limit", &m_controller.m_movementSpeedMax, 0.25f, m_controller.m_movementSpeedMin, 50.0f);

            ImGui::SeparatorText("Camera Info");
            auto& pos = m_controller.m_camera.GetPosition();
            auto& dir = m_controller.m_camera.GetFrontDirection();
            ImGui::Text("Position: %f %f %f", pos.x, pos.y, pos.z);
            ImGui::Text("Direction: %f %f %f", dir.x, dir.y, dir.z);

            ImGui::End();
        }
    }
} // shift::ctrl