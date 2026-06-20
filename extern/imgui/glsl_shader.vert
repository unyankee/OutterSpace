#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require

struct ImguiVertexData
{
    float pos_x;
    float pos_y;
    float uv_x;
    float uv_y;
    uint col;
};

layout(buffer_reference, std430) readonly buffer ImguiVertexDataPtr
{
    ImguiVertexData ImguiVertex[];
};

layout(push_constant) uniform uPushConstant {
    vec2 uScale;
    vec2 uTranslate;
    uint64_t vertexBufferAddress;
} pc;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec4 Color;
    vec2 UV;
} Out;

void main()
{
    ImguiVertexData VertexData = ImguiVertexDataPtr(pc.vertexBufferAddress).ImguiVertex[gl_VertexIndex];
    
    Out.Color = unpackUnorm4x8(VertexData.col);
    Out.UV = vec2(VertexData.uv_x, VertexData.uv_y);
    gl_Position = vec4(vec2(VertexData.pos_x, VertexData.pos_y) * pc.uScale + pc.uTranslate, 0, 1);
}
