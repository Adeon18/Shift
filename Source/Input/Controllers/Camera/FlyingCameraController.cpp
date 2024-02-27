#include "FlyingCameraController.hpp"
#include <glm/gtx/string_cast.hpp>

#include <iostream>

namespace shift::ctrl {
    FlyingCameraController::FlyingCameraController(float fovDeg,
                                                   std::pair<uint32_t, uint32_t> screenWH,
                                                   const glm::vec3 &position)
            : m_camera{fovDeg, screenWH.first, screenWH.second, position} {}

    void FlyingCameraController::CaptureInputAndApply() {
        if (inp::Keyboard::GetInstance().IsPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
            HandleRotation();
            HandleMovement();
        }
    }

    void FlyingCameraController::UpdateScreenSize(float screenWidth,
                                                  float screenHeight) {
        m_camera.UpdateScreenSize(screenWidth, screenHeight);
    }

    void FlyingCameraController::MoveCamera(const glm::vec3 &offset) {
        m_camera.AddRelativeOffset(offset);
    }

    void FlyingCameraController::HandleRotation() {

        glm::vec3 rotation{0.0f, 0.0f, 0.0f};

        rotation.x -= inp::Mouse::GetInstance().GetYMovement() * ROTATION_SPEED * m_camera.GetScreenRatio();
        rotation.y += inp::Mouse::GetInstance().GetXMovement() * ROTATION_SPEED * m_camera.GetScreenRatio();

        if (glm::length(rotation) > 0) {
            m_camera.AddRotation(rotation);
        }
    }

    void FlyingCameraController::HandleMovement() {
        glm::vec3 direction{0.0f, 0.0f, 0.0f};

        for (auto& [k, v]: MOVEMENT_BIND_MAP) {
            if (inp::Keyboard::GetInstance().IsPressed(k)) {
                direction += v;
            }
        }
        if (glm::length(direction) > 0) {
            m_camera.AddRelativeOffset(glm::normalize(direction) * MOVEMENT_SPEED);
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
} // shift::ctrl