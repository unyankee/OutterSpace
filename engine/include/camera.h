#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

// needs refactor, input system, move variables that dont belong here.... a shitload of stuff...

class Camera 
{
public:

	Camera();
	void set_window_dimensions(const uint32_t width, const uint32_t height);
	// will init the camera data, using the already provided info
	void init();

	void update(const float dt);
	void mouse_move(const int32_t x, const int32_t y);

	struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouseButtons;

	struct
	{
		bool left = false;
		bool right = false;
		bool forward = false;
		bool backward = false;
		bool up = false;
		bool down = false;
	} keys;

	bool moving()
	{
		return keys.left || keys.right || keys.forward || keys.backward || keys.down || keys.up;
	}

	void update_view_matrix() { init(); };

	glm::mat4 perspective;
	glm::mat4 view;
	
	float fov = 90.0f;
	float znear = 0.1f;
	float zfar = 1000.0;

	glm::vec3 rotation = glm::vec3();
	glm::vec3 m_location = glm::vec3(10.0f, 10.0f, 10.0f);
	glm::vec3 m_camera_forward = glm::vec3();
	

	// duplicated data, need to get it from elsewhere... >.>
	uint32_t m_width = 1280;
	uint32_t m_height = 720;
	
	// should not be here... but anyways... testing purposes
	glm::vec2 mousePos;


	float movementMultiplier = 1.0f;
	float movementSpeed = 10.0f;
	float rotationSpeed = 1.0f;
};