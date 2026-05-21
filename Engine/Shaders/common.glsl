#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_gpu_shader_int64 : require

struct Meshlet
{
    uint vertexOffset;
    uint triangleOffset;
    uint vertexCount;
    uint triangleCount;

    vec4 centerRadius;
    vec4 coneApexCutoff;
    vec4 coneAxis;
};

struct CameraData
{
    mat4 view;
    mat4 proj;
    vec3 eyePos;
    float padding;
};

layout(push_constant) uniform Constants 
{
    uint64_t vertexBufferAddress; 
    uint64_t cameraBufferAddress; 
    uint64_t meshletBufferAddress;
    uint64_t meshletVertexBufferAddress;
    uint64_t meshletTriangleBufferAddress;
    uint textureIndex;            
    uint samplerIndex;           
} push;

layout(buffer_reference, std430) readonly buffer CameraBufferPtr
{
    CameraData camera;
};

layout(buffer_reference, std430) readonly buffer MeshletBufferPtr
{
    Meshlet meshlets[];
};
