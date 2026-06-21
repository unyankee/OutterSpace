#pragma once
#include "Common/Common.h"

namespace ToyEngine
{

    constexpr float DefaultFarplane = 3000.0f;
    constexpr float DefaultNearPlane = 0.01f;
    

    enum CameraMovement
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    class Camera
    {
    public:
        Camera();

        void setPosition(glm::vec3 position);

        void setTarget(glm::vec3 target);

        void setPerspective(float fovDeg, float aspect, float nearZ = DefaultNearPlane, float farZ = DefaultFarplane);

        const glm::vec3& getPosition() const
        {
            return m_position;
        }

        const glm::mat4& getViewMatrix() const
        {
            return m_viewMatrix;
        }

        const glm::mat4& getProjectionMatrix() const
        {
            return m_projMatrix;
        }

        void update();

        void processKeyboard(CameraMovement direction, float deltaTime);

        void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

        void processMouseScroll(float yoffset);

    private:
        glm::vec3 m_position;
        glm::vec3 m_front;
        glm::vec3 m_up;
        glm::vec3 m_right;
        glm::vec3 m_worldUp;

        // Euler Angles
        float m_yaw;
        float m_pitch;

        // Camera options
        float m_movementSpeed;
        float m_mouseSensitivity;

        glm::mat4 m_viewMatrix;
        glm::mat4 m_projMatrix;

        void updateCameraVectors();

        void updateViewMatrix();
    };

}
