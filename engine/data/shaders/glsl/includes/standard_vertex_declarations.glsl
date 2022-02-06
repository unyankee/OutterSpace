
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

// might replace ssbo with instanced UBO, but that means this instanced buffer will be 
// limited to 1024 instances 
// nvm, I was stupid, obvioulsy for static instanced geo, we want this to be as a vertex input per instance mat4
//layout (location = 2) in mat4 inWorldMatrx;



layout (std140, set = 0, binding = 0) uniform scene_data 
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	// might remove camera position, since this info is in view matrix
	vec4 camera_position; // camera position xyz, time camera_position.w
};

layout (std140, binding = 1) restrict readonly buffer instance_data 
{
	mat4 modelMatrix[];
};

/*layout (std140, binding = 2) restrict readonly buffer indirect_instance_data 
{
	unsigned int instance_data_id;
};*/