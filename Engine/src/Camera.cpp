#include "Camera.h"
#include <cmath>
#include <cstring>

namespace ToyEngine
{
    // Helper math functions
    static Vec3 normalize(Vec3 v) {
        float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len > 0.0001f) {
            return {v.x / len, v.y / len, v.z / len};
        }
        return {0, 0, 0};
    }

    static Vec3 cross(Vec3 a, Vec3 b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    Camera::Camera()
        : m_position({80.0f, 20.0f, 80.0f})
        , m_worldUp({0.0f, 1.0f, 0.0f})
        , m_yaw(-135.0f)
        , m_pitch(-15.0f)
        , m_movementSpeed(10.0f)
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
        Vec3 dir = normalize({target.x - m_position.x, target.y - m_position.y, target.z - m_position.z});
        m_pitch = std::asin(dir.y) * 180.0f / 3.1415926535f;
        m_yaw = std::atan2(dir.z, dir.x) * 180.0f / 3.1415926535f;
        updateCameraVectors();
    }

    void Camera::setPerspective(float fovDeg, float aspect, float nearZ, float farZ) {
        float fovRad = fovDeg * 3.1415926535f / 180.0f;
        float tanHalfFov = std::tan(fovRad / 2.0f);

        std::memset(&m_projMatrix, 0, sizeof(Mat4));
        m_projMatrix.m[0][0] = 1.0f / (aspect * tanHalfFov);
        m_projMatrix.m[1][1] = 1.0f / tanHalfFov;
        m_projMatrix.m[2][2] = farZ / (nearZ - farZ);
        m_projMatrix.m[2][3] = -1.0f;
        m_projMatrix.m[3][2] = (nearZ * farZ) / (nearZ - farZ);
    }


    void Camera::update()
    {
        updateViewMatrix();
    }

    void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        if (direction == FORWARD) {
            m_position.x += m_front.x * velocity;
            m_position.y += m_front.y * velocity;
            m_position.z += m_front.z * velocity;
        }
        if (direction == BACKWARD) {
            m_position.x -= m_front.x * velocity;
            m_position.y -= m_front.y * velocity;
            m_position.z -= m_front.z * velocity;
        }
        if (direction == LEFT) {
            m_position.x -= m_right.x * velocity;
            m_position.y -= m_right.y * velocity;
            m_position.z -= m_right.z * velocity;
        }
        if (direction == RIGHT) {
            m_position.x += m_right.x * velocity;
            m_position.y += m_right.y * velocity;
            m_position.z += m_right.z * velocity;
        }
        if (direction == UP) {
            m_position.y += velocity;
        }
        if (direction == DOWN) {
            m_position.y -= velocity;
        }
    }

    void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
        xoffset *= m_mouseSensitivity;
        yoffset *= m_mouseSensitivity;

        m_yaw += xoffset;
        m_pitch += yoffset;

        if (constrainPitch) {
            if (m_pitch > 89.0f) m_pitch = 89.0f;
            if (m_pitch < -89.0f) m_pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void Camera::updateCameraVectors() {
        Vec3 front;
        float yawRad = m_yaw * 3.1415926535f / 180.0f;
        float pitchRad = m_pitch * 3.1415926535f / 180.0f;
        front.x = std::cos(yawRad) * std::cos(pitchRad);
        front.y = std::sin(pitchRad);
        front.z = std::sin(yawRad) * std::cos(pitchRad);
        m_front = normalize(front);

        m_right = normalize(cross(m_front, m_worldUp));
        m_up = normalize(cross(m_right, m_front));
    }

    void Camera::updateViewMatrix()
    {
        Vec3 x = m_right;
        Vec3 y = m_up;
        Vec3 z = {-m_front.x, -m_front.y, -m_front.z};

        m_viewMatrix.m[0][0] = x.x;
        m_viewMatrix.m[1][0] = x.y;
        m_viewMatrix.m[2][0] = x.z;
        m_viewMatrix.m[3][0] = -(x.x * m_position.x + x.y * m_position.y + x.z * m_position.z);

        m_viewMatrix.m[0][1] = y.x;
        m_viewMatrix.m[1][1] = y.y;
        m_viewMatrix.m[2][1] = y.z;
        m_viewMatrix.m[3][1] = -(y.x * m_position.x + y.y * m_position.y + y.z * m_position.z);

        m_viewMatrix.m[0][2] = z.x;
        m_viewMatrix.m[1][2] = z.y;
        m_viewMatrix.m[2][2] = z.z;
        m_viewMatrix.m[3][2] = -(z.x * m_position.x + z.y * m_position.y + z.z * m_position.z);

        m_viewMatrix.m[0][3] = 0.0f;
        m_viewMatrix.m[1][3] = 0.0f;
        m_viewMatrix.m[2][3] = 0.0f;
        m_viewMatrix.m[3][3] = 1.0f;
    }
}
