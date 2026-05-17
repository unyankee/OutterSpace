#include "Camera.h"
#include <cmath>
#include <cstring>

namespace ToyEngine
{
    Camera::Camera()
        : m_position({80.0f, 20.0f, 80.0f})
          , m_target({0.0f, 0.0f, 0.0f})
          , m_up({0.0f, 1.0f, 0.0f})
    {
        update();
    }

    void Camera::setPosition(Vec3 position)
    {
        m_position = position;
    }

    void Camera::setTarget(Vec3 target)
    {
        m_target = target;
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

    void Camera::updateViewMatrix()
    {
        Vec3 z = {m_position.x - m_target.x, m_position.y - m_target.y, m_position.z - m_target.z};
        float lenZ = std::sqrt(z.x * z.x + z.y * z.y + z.z * z.z);
        z.x /= lenZ;
        z.y /= lenZ;
        z.z /= lenZ;

        Vec3 x = {m_up.y * z.z - m_up.z * z.y, m_up.z * z.x - m_up.x * z.z, m_up.x * z.y - m_up.y * z.x};
        float lenX = std::sqrt(x.x * x.x + x.y * x.y + x.z * x.z);
        x.x /= lenX;
        x.y /= lenX;
        x.z /= lenX;

        Vec3 y = {z.y * x.z - z.z * x.y, z.z * x.x - z.x * x.z, z.x * y.y - z.y * x.x}; 
        y = {z.y * x.z - z.z * x.y, z.z * x.x - z.x * x.z, z.x * x.y - z.y * x.x};

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
