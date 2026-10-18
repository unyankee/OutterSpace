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

struct TextureCollectionData
{
    uint textures[8];
    uint textureCount;
    uint padding[3];
};

struct MaterialData
{
    TextureCollectionData textureCollection;
    uint samplerIndex;
};

struct SuppliesIndirectionData
{
    MaterialData materialData;
    uint transformIdx;
    uint meshID;
};
struct MeshRegistryData
{
    uint meshletCount;
    uint vertexOffset;
    uint meshletOffset;
    uint meshletVertexOffset;
    uint meshletTriangleOffset;
};

struct TransformData
{
    mat4x3 modelMatrix;
};

mat4 expandTransform(TransformData transform)
{
    return mat4(
        vec4(transform.modelMatrix[0], 0.0),
        vec4(transform.modelMatrix[1], 0.0),
        vec4(transform.modelMatrix[2], 0.0),
        vec4(transform.modelMatrix[3], 1.0));
}


// I think might moved this one out of common?
// might be somethig added at the top of the shaders that needs it?
layout(push_constant) uniform Constants
{
    uint64_t vertexBufferAddress;
    uint64_t cameraBufferAddress;
    uint64_t meshletBufferAddress;
    uint64_t meshletVertexBufferAddress;
    uint64_t meshletTriangleBufferAddress;
    uint64_t TransformDataAddress;
    uint64_t SuppliesIndirectionDataAddress;
    uint64_t MeshRegistryDataAddress;
    uint InstanceID;
} push; // 128 max

layout(buffer_reference, std430) readonly buffer CameraBufferPtr
{
    CameraData camera;
};

layout(buffer_reference, std430) readonly buffer MeshletBufferPtr
{
    Meshlet meshlets[];
};

layout(buffer_reference, scalar) readonly buffer SuppliesIndirectionDataPtr
{
    SuppliesIndirectionData entries[];
};
layout(buffer_reference, scalar) readonly buffer MeshRegistryDataPtr
{
    MeshRegistryData entries[];
};
layout(buffer_reference, scalar) readonly buffer TransformDataPtr
{
    TransformData transforms[];
};
