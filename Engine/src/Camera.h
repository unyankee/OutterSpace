#pragma once

namespace ToyEngine
{

    constexpr float DefaultFarplane = 3000.0f;
    constexpr float DefaultNearPlane = 0.1f;
    
    struct Vec3
    {
        float m_x;
        float m_y;
        float m_z;
    };

    struct Mat4
    {
        float m_m[4][4];
    };

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

        void setPosition(Vec3 position);

        void setTarget(Vec3 target);

        void setPerspective(float fovDeg, float aspect, float nearZ = DefaultNearPlane, float farZ = DefaultFarplane);

        const Vec3& getPosition() const
        {
            return m_position;
        }

        const Mat4& getViewMatrix() const
        {
            return m_viewMatrix;
        }

        const Mat4& getProjectionMatrix() const
        {
            return m_projMatrix;
        }

        void update();

        void processKeyboard(CameraMovement direction, float deltaTime);

        void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    private:
        Vec3 m_position;
        Vec3 m_front;
        Vec3 m_up;
        Vec3 m_right;
        Vec3 m_worldUp;

        // Euler Angles
        float m_yaw;
        float m_pitch;

        // Camera options
        float m_movementSpeed;
        float m_mouseSensitivity;

        Mat4 m_viewMatrix;
        Mat4 m_projMatrix;

        void updateCameraVectors();

        void updateViewMatrix();
    };

}
