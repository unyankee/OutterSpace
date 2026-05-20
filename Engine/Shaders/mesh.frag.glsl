#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require 
#extension GL_ARB_gpu_shader_int64 : require    

#include "common.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 outMeshletDebugColor;

layout(set = 0, binding = 0) uniform texture2D globalTextures[];
layout(set = 0, binding = 1) uniform sampler globalSamplers[];


layout(location = 0) out vec4 outColor;
void main()
{
    vec4 texColor = texture(
        sampler2D(
            globalTextures[nonuniformEXT(push.textureIndex)], 
            globalSamplers[nonuniformEXT(push.samplerIndex)]
        ), 
        inUV
    );

    float lightIntensity = max(dot(normalize(inNormal), normalize(vec3(0.5, 1.0, 0.3))), 0.2);

    outColor = vec4(texColor.rgb * lightIntensity, texColor.a);
    outColor = vec4(outMeshletDebugColor.rgb * lightIntensity, texColor.a);
}