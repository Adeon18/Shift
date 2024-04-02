#ifndef SHIFT_FLYINGCAMERACONTROLLER_HPP
#define SHIFT_FLYINGCAMERACONTROLLER_HPP

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <array>

#include <glm/glm.hpp>

#include "GLFW/glfw3.h"
#include "Input/Mouse.hpp"
#include "Graphics/Camera/EulerCamera.hpp"

#include "Graphics/UI/UIWindowComponent.hpp"

namespace shift::ctrl {
    class FlyingCameraController {
        class UI: public gfx::ui::UIWindowComponent {
        public:
            explicit UI(std::string name, std::string sName, FlyingCameraController& system): gfx::ui::UIWindowComponent{std::move(name), std::move(sName)}, m_controller{system} {
            }

            virtual void Item() override { gfx::ui::UIWindowComponent::Item(); }
            virtual void Show(uint32_t currentFrame) override;

        private:
            FlyingCameraController& m_controller;
        };

        static constexpr float ROTATION_SPEED = 1.0f;

        inline static std::unordered_map<int, glm::vec3> MOVEMENT_BIND_MAP{
                {GLFW_KEY_A, {-1.0f, 0.0f, 0.0f}},
                {GLFW_KEY_D, {1.0f, 0.0f, 0.0f}},
                {GLFW_KEY_W, {0.0f, 0.0f, 1.0f}},
                {GLFW_KEY_S, {0.0f, 0.0f, -1.0f}},
                {GLFW_KEY_Q, {0.0f, -1.0f, 0.0f}},
                {GLFW_KEY_E, {0.0f, 1.0f, 0.0f}},
        };
    public:
        FlyingCameraController(float fovDeg, std::pair<uint32_t, uint32_t> screenWH,
                               const glm::vec3 &position);

        FlyingCameraController() = default;
        FlyingCameraController(const FlyingCameraController&) = default;
        FlyingCameraController& operator=(const FlyingCameraController&) = default;

        //! Capture the input from the players mouse and keyboard and apply the
        //! respective movement to the camera
        void CaptureInputAndApply(float dt);

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
        UI m_UI{"Camera Settings", "Settings", *this};
        gfx::EulerCamera m_camera;

        float m_movementSpeed = 3.0f;
        float m_movementSpeedChange = 0.25f;
        float m_movementSpeedMin = 0.1f;
        float m_movementSpeedMax = 20.0f;
    private:
        //! Handle Camera rotation
        void HandleRotation(float dt);

        //! Handle Camera rotation
        void HandleMovement(float dt);
    };
} // shift::ctrl
#endif //SHIFT_FLYINGCAMERACONTROLLER_HPP
