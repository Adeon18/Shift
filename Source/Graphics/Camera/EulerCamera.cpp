#include <glm/gtx/euler_angles.hpp>
#include <iostream>
#include <algorithm>

#include "EulerCamera.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtx/string_cast.hpp"

namespace shift::gfx {
    EulerCamera::EulerCamera(float fov, uint32_t screenWidth, uint32_t screenHeight,
                   const glm::vec3 &position)
            : m_fov{fov}, m_position{position}, m_rotation{0.0f, 0.0f, 0.0f}, m_width{screenWidth}, m_height{screenHeight} {
        m_ratio = static_cast<float>(m_width) / static_cast<float>(m_height);
        SetProjectionMatrix(fov);
        UpdateViewMatrix();
    }

    void EulerCamera::SetProjectionMatrix(float fov) {
        m_projection = glm::perspective(glm::radians(fov),
                                        m_ratio,
                                        0.1f, 1000.0f);
    }

    void EulerCamera::UpdateScreenSize(float screenWidth, float screenHeight) {
        m_width = static_cast<uint32_t>(screenWidth); m_height = static_cast<uint32_t>(screenHeight);
        m_ratio = static_cast<float>(m_width) / static_cast<float>(m_height);
        SetProjectionMatrix(m_fov);
    }

    void EulerCamera::AddRotation(const glm::vec3 &angles) {
        m_rotation += angles;
        m_rotation.x = std::clamp(m_rotation.x, -1.5708f, 1.5708f); // 1.5708 is 90 degrees in radians
        if (m_rotation.x >= glm::pi<float>()) {
            m_rotation.x = glm::pi<float>();
        } else if (m_rotation.x <= -glm::pi<float>()) {
            m_rotation.x = glm::pi<float>();
        }
        UpdateViewMatrix();
    }

    void EulerCamera::AddRelativeOffset(const glm::vec3 &offset) {
        m_position +=
                m_rightDir * offset.x + m_upDir * offset.y + m_frontDir * offset.z;
        UpdateViewMatrix();
    }

    void EulerCamera::UpdateViewMatrix() {
        glm::mat4 rotationMat =
                glm::eulerAngleYXZ(m_rotation.y, m_rotation.x, m_rotation.z);

        m_frontDir = rotationMat * glm::vec4(FRONT_DIR, 0.0f);

        m_upDir = rotationMat * glm::vec4(UP_DIR, 0.0f);

        m_rightDir = rotationMat * glm::vec4(RIGHT_DIR, 0.0f);

        m_view = glm::lookAt(m_position, m_position + m_frontDir, m_upDir);
    }
} // shift::gfx