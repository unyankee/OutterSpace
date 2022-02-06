#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;

layout (std140, set = 0, binding = 0) uniform UBO 
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	vec4 camera_position; // camera position xyz, time camera_position.w
} ubo;

layout (std140, binding = 1) restrict readonly buffer INSTANCE_DATA 
{
	mat4 modelMatrix[];
};


layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 directionalLightDir;

out gl_PerVertex 
{
    vec4 gl_Position;
};


// just so it does not look so simple... 
// adding a single directional in the actual shader

const vec3 colours[8] = 
{
	vec3(1.0,0.0,0.0),
	vec3(0.0,1.0,0.0),
	vec3(0.0,0.0,1.0),
	vec3(0.0,1.0,1.0),
	vec3(1.0,1.0,0.0),
	vec3(1.0,0.0,1.0),
	vec3(1.0,1.0,1.0),
	vec3(0.2,1.0,0.5)
};


void main() 
{
	const float time = ubo.camera_position.w; 
	const float z_dir = sin(0.0);
	const float y_dir = cos(0.0) * 0.5 + 0.5;
	const vec3 dir_light = normalize(vec3(-1.0, -y_dir, z_dir));

	mat4 model_matrix = modelMatrix[gl_InstanceIndex];

	gl_Position = ubo.projectionMatrix * ubo.viewMatrix * model_matrix * vec4(inPos.xyz, 1.0);
		
	vec3 diffuse_colour = colours[gl_InstanceIndex % 8];
	const vec4 normal = model_matrix * vec4(inNormal.xyz, 0.0);
	outColor = diffuse_colour;
	outNormal = normalize(normal.xyz);	
	directionalLightDir = dir_light;	
}
