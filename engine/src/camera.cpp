#include "..\include\camera.h"

Camera::Camera()
{
    init();
}

void Camera::set_window_dimensions(const uint32_t width, const uint32_t height)
{
    m_width  = width;
    m_height = height;
}

void Camera::init()
{
    if (rotation.x > 89.0f)
    {
        rotation.x = 89.0f;
    }
    if (rotation.x < -89.0f)
    {
        rotation.x = -89.0f;
    }

    glm::mat4 camera_rotation = glm::mat4(1.0f);
    glm::mat4 camera_position_matrix;

    camera_rotation = glm::rotate(camera_rotation, glm::radians(rotation.x), glm::vec3(-1.0f, 0.0f, 0.0f));
    camera_rotation = glm::rotate(camera_rotation, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    camera_rotation = glm::rotate(camera_rotation, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    // Because the coordinate system Vulkan uses is xz+ / y-... this position needs to be revesed
    // is important to know that the input is also mirrored
    glm::vec3 trans        = -m_location;
    camera_position_matrix = glm::translate(glm::mat4(1.0f), trans);

    view = camera_rotation * camera_position_matrix;

    perspective = glm::perspective(glm::radians(fov), (float(m_width) / float(m_height)), znear, zfar);
    // this m_dirty -1.0f is to flip the viewport, so we do not see positive y values going below
    // what is kinda awkward
    perspective[1][1] *= -1.0f;
}

void Camera::update(const float dt)
{
    if (!mouseButtons.right)
    {
        keys.left = keys.right = keys.forward = keys.backward = keys.down = keys.up = false;
        return;
    }
    if (!moving())
    {
        return;
    }

    glm::vec3 camFront;
    camFront.x       = -cos(glm::radians(-rotation.x)) * sin(glm::radians(rotation.y));
    camFront.y       = sin(glm::radians(-rotation.x));
    camFront.z       = cos(glm::radians(-rotation.x)) * cos(glm::radians(rotation.y));
    camFront         = glm::normalize(camFront);
    m_camera_forward = camFront;

    float moveSpeed = dt * movementSpeed * movementMultiplier;

    if (keys.forward)
    {
        m_location -= m_camera_forward * moveSpeed;
    }
    if (keys.backward)
    {
        m_location += m_camera_forward * moveSpeed;
    }
    if (keys.left)
    {
        m_location += glm::normalize(glm::cross(m_camera_forward, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
    }
    if (keys.right)
    {
        m_location -= glm::normalize(glm::cross(m_camera_forward, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
    }
    if (keys.up)
    {
        m_location.y += moveSpeed;
    }
    if (keys.down)
    {
        m_location.y -= moveSpeed;
    }

    update_view_matrix();
}

void Camera::mouse_move(const int32_t x, const int32_t y)
{
    int32_t dx = (int32_t)mousePos.x - x;
    int32_t dy = (int32_t)mousePos.y - y;

    if (mouseButtons.right)
    {
        rotation += glm::vec3(dy * rotationSpeed, -dx * rotationSpeed, 0.0f);
        init();
    }
    mousePos = glm::vec2((float)x, (float)y);
}
