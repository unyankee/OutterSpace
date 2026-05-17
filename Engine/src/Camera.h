#pragma once

namespace ToyEngine {

    struct Vec3 {
        float x, y, z;
    };

    struct Mat4 {
        float m[4][4];
    };

    class Camera {
    public:
        Camera();
        
        void setPosition(Vec3 position);
        void setTarget(Vec3 target);
        void setPerspective(float fovDeg, float aspect, float nearZ, float farZ);
        
        const Mat4& getViewMatrix() const { return m_viewMatrix; }
        const Mat4& getProjectionMatrix() const { return m_projMatrix; }

        void update();

    private:
        Vec3 m_position;
        Vec3 m_target;
        Vec3 m_up;

        Mat4 m_viewMatrix;
        Mat4 m_projMatrix;

        void updateViewMatrix();
    };

}
