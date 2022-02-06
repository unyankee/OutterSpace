#version 450
//#extension GL_GOOGLE_include_directive : require

#include "includes/standard_vertex_declarations.glsl"

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;

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
	const float time = camera_position.w; 
	const float z_dir = sin(0.0);
	const float y_dir = cos(0.0) * 0.5 + 0.5;
	const vec3 dir_light = normalize(vec3(-1.0, -y_dir, z_dir));

	const mat4 model_matrix = modelMatrix[gl_InstanceIndex];
	const vec4 world_position = model_matrix * vec4(inPos.xyz, 1.0);

	gl_Position = projectionMatrix * viewMatrix * world_position;

	vec3 diffuse_colour = colours[gl_InstanceIndex % 8];
	const vec4 normal = model_matrix * vec4(inNormal.xyz, 0.0);
	outColor = diffuse_colour;
	outNormal = normalize(normal.xyz);	
}
