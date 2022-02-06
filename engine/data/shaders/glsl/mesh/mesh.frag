#version 450


#include "includes/general_extension.glsl"

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;
layout (location = 12) in vec4 pos_from_lighting;


layout (location = 0) out vec4 outFragColor;

//layout (binding = 2) uniform texture2D texture_test;
//layout (binding = 1) uniform sampler sampler_test;


vec3 ambient_light = vec3(0.84,0.133,0.125);
vec3 direct_light_colour = vec3(0.984,0.670,0.01);

void main() 
{
	const vec3 normal = normalize(inNormal);
	outFragColor = vec4(inColor, 1.0);
}