#ifndef SHIFT_EULERCAMERA_HPP
#define SHIFT_EULERCAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace sft::gfx {
    class EulerCamera {
    public:
        EulerCamera(float fov, uint32_t screenWidth, uint32_t screenHeight,
               const glm::vec3 &position);

        EulerCamera() = default;
        EulerCamera(const EulerCamera&) = default;
        EulerCamera& operator=(const EulerCamera&) = default;

        void SetProjectionMatrix(float fov);
        void UpdateScreenSize(float screenWidth, float screenHeight);
        void AddRotation(const glm::vec3 &angles);
        void AddRelativeOffset(const glm::vec3 &offset);

        [[nodiscard]] float GetScreenRatio() const { return m_ratio; }

        [[nodiscard]] glm::mat4 &GetProjectionMatrix() { return m_projection; };
        [[nodiscard]] glm::mat4 &GetViewMatrix() { return m_view; };
        [[nodiscard]] glm::vec3 &GetPosition() { return m_position; };
        [[nodiscard]] glm::vec3 &GetFrontDirection() { return m_frontDir; };
        [[nodiscard]] glm::vec3 &GetRightDirection() { return m_rightDir; };
        [[nodiscard]] glm::vec3 &GetUpDirection() { return m_upDir; };

        void SetPosition(const glm::vec3& pos) {
            m_position = pos;
            UpdateViewMatrix();
        }

    private:
        //! Update view matrix, called at every camera state change
        void UpdateViewMatrix();

        static constexpr glm::vec3 FRONT_DIR = glm::vec3{0.0f, 0.0f, -1.0f};
        static constexpr glm::vec3 UP_DIR = glm::vec3{0.0f, -1.0f, 0.0f};
        static constexpr glm::vec3 RIGHT_DIR = glm::vec3{1.0f, 0.0f, 0.0f};

        float m_fov;

        glm::vec3 m_frontDir;
        glm::vec3 m_rightDir;
        glm::vec3 m_upDir;
        glm::vec3 m_position;

        glm::vec3 m_rotation;

        glm::mat4 m_projection;
        glm::mat4 m_view;

        uint32_t m_width;
        uint32_t m_height;
        float m_ratio;
    };
} // sft::gfx

#endif //SHIFT_EULERCAMERA_HPP
