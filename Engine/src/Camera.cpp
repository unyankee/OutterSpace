#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ToyEngine
{

    Camera::Camera()
        : m_position({80.0f, 20.0f, 80.0f})
        , m_worldUp({0.0f, 1.0f, 0.0f})
        , m_yaw(-135.0f)
        , m_pitch(-15.0f)
        , m_movementSpeed(40.0f)
        , m_mouseSensitivity(0.1f)
    {
        updateCameraVectors();
    }

    void Camera::setPosition(glm::vec3 position)
    {
        m_position = position;
    }

    void Camera::setTarget(glm::vec3 target)
    {
        glm::vec3 dir = glm::normalize(target - m_position);

        m_pitch = glm::degrees(std::asin(dir.y));
        m_yaw   = glm::degrees(std::atan2(dir.z, dir.x));

        updateCameraVectors();
    }

    void Camera::setPerspective(float fovDeg, float aspect, float nearZ, float farZ)
    {
        //m_projMatrix = glm::perspective(glm::radians(fovDeg), aspect, nearZ, farZ);
        // Reverse Depth here
        float f = 1.0f / tan(glm::radians(fovDeg) / 2.0f);
        m_projMatrix =  glm::mat4(
            f / aspect, 0.0f,  0.0f,  0.0f,
                      0.0f,    f,  0.0f,  0.0f,
                      0.0f, 0.0f,  0.0f, -1.0f,
                      0.0f, 0.0f, nearZ,  0.0f);
        
    }

    void Camera::update()
    {
        updateViewMatrix();
    }

    void Camera::processKeyboard(CameraMovement direction, float deltaTime)
    {
        float velocity = m_movementSpeed * deltaTime;

        if (direction == FORWARD)   m_position += m_front * velocity;
        if (direction == BACKWARD)  m_position -= m_front * velocity;
        if (direction == LEFT)      m_position -= m_right * velocity;
        if (direction == RIGHT)     m_position += m_right * velocity;
        if (direction == UP)        m_position.y += velocity;
        if (direction == DOWN)      m_position.y -= velocity;
    }

    void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
    {
        xoffset *= m_mouseSensitivity;
        yoffset *= m_mouseSensitivity;

        m_yaw   += xoffset;
        m_pitch += yoffset;

        if (constrainPitch)
        {
            if (m_pitch >  89.0f) m_pitch =  89.0f;
            if (m_pitch < -89.0f) m_pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void Camera::processMouseScroll(float yoffset)
    {
        m_movementSpeed += yoffset * 2.0f;
        if (m_movementSpeed < 1.0f)
            m_movementSpeed = 1.0f;
    }

    void Camera::updateCameraVectors()
    {
        glm::vec3 front;
        float yawRad   = glm::radians(m_yaw);
        float pitchRad = glm::radians(m_pitch);

        front.x = std::cos(yawRad) * std::cos(pitchRad);
        front.y = std::sin(pitchRad);
        front.z = std::sin(yawRad) * std::cos(pitchRad);

        m_front = glm::normalize(front);
        m_right = glm::normalize(glm::cross(m_front, m_worldUp));
        m_up    = glm::normalize(glm::cross(m_right, m_front));
    }

    void Camera::updateViewMatrix()
    {
        m_viewMatrix = glm::lookAt(m_position, m_position + m_front, m_up);
    }

}