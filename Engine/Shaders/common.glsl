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

struct TransformData
{
    vec4 m_position;
    vec4 m_rotation;
    vec4 m_scale;
    mat4x4 modelMatrix;
};


// I think might moved this one out of common?
// might be somethig added at the top of the shaders that needs it?
layout(push_constant) uniform Constants 
{
    uint64_t vertexBufferAddress;           // 8   @ 0
    uint64_t cameraBufferAddress;           // 8   @ 8
    uint64_t meshletBufferAddress;          // 8   @ 16
    uint64_t meshletVertexBufferAddress;    // 8   @ 24
    uint64_t meshletTriangleBufferAddress;  // 8   @ 32
    uint64_t TransformDataAddress;          // 8   @ 40
    uint     textureIndex;                  // 4   @ 44
    uint     samplerIndex;                  // 4   @ 48
    uint     meshletCount;                  // 4   @ 52
    uint     TransformIndex;                // 4   @ 56
} push; // 128 max

layout(buffer_reference, std430) readonly buffer CameraBufferPtr
{
    CameraData camera;
};

layout(buffer_reference, std430) readonly buffer MeshletBufferPtr
{
    Meshlet meshlets[];
};

layout(buffer_reference, std430) readonly buffer TransformDataPtr
{
    TransformData transforms[];
};
