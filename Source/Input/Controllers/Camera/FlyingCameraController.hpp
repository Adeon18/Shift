#ifndef SHIFT_FLYINGCAMERACONTROLLER_HPP
#define SHIFT_FLYINGCAMERACONTROLLER_HPP

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "GLFW/glfw3.h"
#include "Input/Mouse.hpp"
#include "Graphics/Camera/EulerCamera.hpp"

namespace sft::ctrl {
    class FlyingCameraController {
        static constexpr float ROTATION_SPEED = 0.001f;
    public:
        FlyingCameraController(float fovDeg, std::pair<uint32_t, uint32_t> screenWH,
                               const glm::vec3 &position);

        FlyingCameraController() = default;
        FlyingCameraController(const FlyingCameraController&) = default;
        FlyingCameraController& operator=(const FlyingCameraController&) = default;

        //! Capture the input from the players mouse and keyboard and apply the
        //! respective movement to the camera
        void CaptureInputAndApply();

        //! Update the projection matrix of internal camera to fit the new screen
        //! size at resize
        void UpdateScreenSize(float screenWidth, float screenHeight);

        glm::vec3 GetPosition();

        glm::vec3 GetDirection();

        glm::vec3 GetRightDir();

        glm::vec3 GetUpDir();

        void SetPosition(const glm::vec3 &pos) {
            m_camera.SetPosition(pos);
        }

        void MoveCamera(const glm::vec3 &offset);

        gfx::EulerCamera &GetCamera() { return m_camera; }

    private:
        gfx::EulerCamera m_camera;

    private:
        //! Handle Camera rotation
        void HandleRotation();
    };
} // sft::ctrl
#endif //SHIFT_FLYINGCAMERACONTROLLER_HPP
