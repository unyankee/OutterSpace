#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inDirLight;

layout (location = 0) out vec4 outFragColor;


vec3 ambient_light = vec3(0.84,0.133,0.125);
vec3 direct_light_colour = vec3(0.984,0.670,0.01);


void main() 
{
	const vec3 normal = normalize(inNormal);
	
	const float attenuation = dot(normalize(normal.xyz), -inDirLight);
	vec3 direct_colour = attenuation * direct_light_colour;

	outFragColor = vec4((direct_colour + ambient_light) * inColor, 1.0);
}