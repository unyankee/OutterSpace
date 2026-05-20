#include "Camera.h"

#include <cmath>
#include <cstring>

namespace ToyEngine
{

    // Helper math functions
    static Vec3 normalize(Vec3 v)
    {
        float len = std::sqrt(v.m_x * v.m_x + v.m_y * v.m_y + v.m_z * v.m_z);

        if (len > 0.0001f)
        {
            return {v.m_x / len, v.m_y / len, v.m_z / len};
        }

        return {0, 0, 0};
    }

    static Vec3 cross(Vec3 a, Vec3 b)
    {
        return {
            a.m_y * b.m_z - a.m_z * b.m_y,
            a.m_z * b.m_x - a.m_x * b.m_z,
            a.m_x * b.m_y - a.m_y * b.m_x
        };
    }

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

    void Camera::setPosition(Vec3 position)
    {
        m_position = position;
    }

    void Camera::setTarget(Vec3 target)
    {
        // Calculate yaw and pitch from target if needed, but for now just update vectors
        Vec3 dir = normalize({target.m_x - m_position.m_x, target.m_y - m_position.m_y, target.m_z - m_position.m_z});

        m_pitch = std::asin(dir.m_y) * 180.0f / 3.1415926535f;
        m_yaw = std::atan2(dir.m_z, dir.m_x) * 180.0f / 3.1415926535f;

        updateCameraVectors();
    }

    void Camera::setPerspective(float fovDeg, float aspect, float nearZ, float farZ)
    {
        float fovRad = fovDeg * 3.1415926535f / 180.0f;
        float tanHalfFov = std::tan(fovRad / 2.0f);

        std::memset(&m_projMatrix, 0, sizeof(Mat4));

        m_projMatrix.m_m[0][0] = 1.0f / (aspect * tanHalfFov);
        m_projMatrix.m_m[1][1] = 1.0f / tanHalfFov;
        m_projMatrix.m_m[2][2] = farZ / (nearZ - farZ);
        m_projMatrix.m_m[2][3] = -1.0f;
        m_projMatrix.m_m[3][2] = (nearZ * farZ) / (nearZ - farZ);
    }

    void Camera::update()
    {
        updateViewMatrix();
    }

    void Camera::processKeyboard(CameraMovement direction, float deltaTime)
    {
        float velocity = m_movementSpeed * deltaTime;

        if (direction == FORWARD)
        {
            m_position.m_x += m_front.m_x * velocity;
            m_position.m_y += m_front.m_y * velocity;
            m_position.m_z += m_front.m_z * velocity;
        }

        if (direction == BACKWARD)
        {
            m_position.m_x -= m_front.m_x * velocity;
            m_position.m_y -= m_front.m_y * velocity;
            m_position.m_z -= m_front.m_z * velocity;
        }

        if (direction == LEFT)
        {
            m_position.m_x -= m_right.m_x * velocity;
            m_position.m_y -= m_right.m_y * velocity;
            m_position.m_z -= m_right.m_z * velocity;
        }

        if (direction == RIGHT)
        {
            m_position.m_x += m_right.m_x * velocity;
            m_position.m_y += m_right.m_y * velocity;
            m_position.m_z += m_right.m_z * velocity;
        }

        if (direction == UP)
        {
            m_position.m_y += velocity;
        }

        if (direction == DOWN)
        {
            m_position.m_y -= velocity;
        }
    }

    void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch)
    {
        xoffset *= m_mouseSensitivity;
        yoffset *= m_mouseSensitivity;

        m_yaw += xoffset;
        m_pitch += yoffset;

        if (constrainPitch)
        {
            if (m_pitch > 89.0f)
            {
                m_pitch = 89.0f;
            }

            if (m_pitch < -89.0f)
            {
                m_pitch = -89.0f;
            }
        }

        updateCameraVectors();
    }

    void Camera::updateCameraVectors()
    {
        Vec3 front;
        float yawRad = m_yaw * 3.1415926535f / 180.0f;
        float pitchRad = m_pitch * 3.1415926535f / 180.0f;

        front.m_x = std::cos(yawRad) * std::cos(pitchRad);
        front.m_y = std::sin(pitchRad);
        front.m_z = std::sin(yawRad) * std::cos(pitchRad);

        m_front = normalize(front);
        m_right = normalize(cross(m_front, m_worldUp));
        m_up = normalize(cross(m_right, m_front));
    }

    void Camera::updateViewMatrix()
    {
        Vec3 x = m_right;
        Vec3 y = m_up;
        Vec3 z = {-m_front.m_x, -m_front.m_y, -m_front.m_z};

        m_viewMatrix.m_m[0][0] = x.m_x;
        m_viewMatrix.m_m[1][0] = x.m_y;
        m_viewMatrix.m_m[2][0] = x.m_z;
        m_viewMatrix.m_m[3][0] = -(x.m_x * m_position.m_x + x.m_y * m_position.m_y + x.m_z * m_position.m_z);

        m_viewMatrix.m_m[0][1] = y.m_x;
        m_viewMatrix.m_m[1][1] = y.m_y;
        m_viewMatrix.m_m[2][1] = y.m_z;
        m_viewMatrix.m_m[3][1] = -(y.m_x * m_position.m_x + y.m_y * m_position.m_y + y.m_z * m_position.m_z);

        m_viewMatrix.m_m[0][2] = z.m_x;
        m_viewMatrix.m_m[1][2] = z.m_y;
        m_viewMatrix.m_m[2][2] = z.m_z;
        m_viewMatrix.m_m[3][2] = -(z.m_x * m_position.m_x + z.m_y * m_position.m_y + z.m_z * m_position.m_z);

        m_viewMatrix.m_m[0][3] = 0.0f;
        m_viewMatrix.m_m[1][3] = 0.0f;
        m_viewMatrix.m_m[2][3] = 0.0f;
        m_viewMatrix.m_m[3][3] = 1.0f;
    }

}
