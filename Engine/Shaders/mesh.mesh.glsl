#version 460
#extension GL_EXT_mesh_shader : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require
#extension GL_KHR_shader_subgroup_ballot : require

#include "common.glsl"

const uint MeshletMaxVertices = 64;
const uint MeshletMaxTriangles = 124;

struct Vertex
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tu, tv;
};

layout(buffer_reference, std430) readonly buffer VertexBufferPtr
{
    Vertex vertices[];
};

layout(buffer_reference, std430) readonly buffer MeshletVertexBufferPtr
{
    uint vertexIndices[];
};

layout(buffer_reference, std430) readonly buffer MeshletTriangleBufferPtr
{
    uint triangleIndices[];
};

taskPayloadSharedEXT struct TaskPayload
{
    uint meshletIndices[32];
} payload;

layout(local_size_x = 32) in;
layout(triangles, max_vertices = 64, max_primitives = 124) out;

layout(location = 0) out vec2 outUV[];
layout(location = 1) out vec3 outNormal[];
// I want this one just for debugging the meshlets
layout(location = 2) out vec3 outMeshletDebugColor[];

// hash for random debug color based on the id of the meshlet
uint pcg(uint seed)
{
    uint state = seed * 747796405u + 2891336453u;
    uint word  = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

vec3 randomColor(uint seed)
{
    return vec3(
        float(pcg(seed))           / float(0xFFFFFFFFu),
        float(pcg(seed ^ 12345u))  / float(0xFFFFFFFFu),
        float(pcg(seed * 6789u))   / float(0xFFFFFFFFu)
    );
}

void main()
{
    const uint TransformIndex = push.TransformIndex;
    TransformData transform = TransformDataPtr(push.TransformDataAddress).transforms[TransformIndex];
    
    uint meshletIndex = payload.meshletIndices[gl_WorkGroupID.x];
    Meshlet meshlet = MeshletBufferPtr(push.meshletBufferAddress).meshlets[meshletIndex];
    SetMeshOutputsEXT(meshlet.vertexCount, meshlet.triangleCount);

    uint localIndex = gl_LocalInvocationIndex;
    CameraData cam = CameraBufferPtr(push.cameraBufferAddress).camera;

    for (uint i = localIndex; i < meshlet.vertexCount; i += gl_WorkGroupSize.x)
    {
        uint vertexIndex = MeshletVertexBufferPtr(push.meshletVertexBufferAddress).vertexIndices[meshlet.vertexOffset + i];
        Vertex vertex = VertexBufferPtr(push.vertexBufferAddress).vertices[vertexIndex];

        vec3 position = vec3(vertex.vx, vertex.vy, vertex.vz);
        gl_MeshVerticesEXT[i].gl_Position = cam.proj * cam.view* transform.modelMatrix * vec4(position, 1.0);
        
        outUV[i] = vec2(vertex.tu, vertex.tv);
        outNormal[i] = vec3(vertex.nx, vertex.ny, vertex.nz);

        // since the whole meshlet is in the same group... 
        const vec3 debugColor = subgroupBroadcastFirst(randomColor(meshletIndex));
        outMeshletDebugColor[i] = debugColor;
    }

    for (uint i = localIndex; i < meshlet.triangleCount; i += gl_WorkGroupSize.x)
    {
        uint triangleOffset = meshlet.triangleOffset + i * 3;
        gl_PrimitiveTriangleIndicesEXT[i] = uvec3(
            MeshletTriangleBufferPtr(push.meshletTriangleBufferAddress).triangleIndices[triangleOffset + 0],
            MeshletTriangleBufferPtr(push.meshletTriangleBufferAddress).triangleIndices[triangleOffset + 1],
            MeshletTriangleBufferPtr(push.meshletTriangleBufferAddress).triangleIndices[triangleOffset + 2]);
    }
}
