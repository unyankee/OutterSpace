#version 450 core
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : require
layout(location = 0) out vec4 fColor;

layout(set = 2, binding = 0) uniform texture2D globalTextures[];
layout(set = 2, binding = 1) uniform sampler globalSamplers[];
layout(set = 1, binding = 0, scalar) uniform uPushConstant {
    vec2 uScale;
    vec2 uTranslate;
    uint64_t vertexBufferAddress;
    uint textureIndex;
} pc;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

void main()
{
    fColor = In.Color * texture(sampler2D(globalTextures[nonuniformEXT(pc.textureIndex)], globalSamplers[0]), In.UV.st);
}
