#version 450
#extension GL_EXT_buffer_reference : require


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

struct CameraData {
    mat4 view;
    mat4 proj;
};

layout(buffer_reference, std430) readonly buffer CameraBufferPtr 
{
    CameraData camera;
};

layout(push_constant) uniform Constants 
{
    VertexBufferPtr vertexBufferAddress; 
    CameraBufferPtr cameraBufferAddress;
    uint textureIndex;
} push;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;

void main()
{
	// Get the vertex data
    Vertex vertex = push.vertexBufferAddress.vertices[gl_VertexIndex];
    
    vec3 position = vec3(vertex.vx, vertex.vy, vertex.vz);
    vec3 normal = vec3(vertex.nx, vertex.ny, vertex.nz);
    vec2 texCoord = vec2(vertex.tu, vertex.tv);

    CameraData cam = push.cameraBufferAddress.camera;

    gl_Position = cam.proj * cam.view * vec4(position, 1.0);

    outUV = texCoord;
    outNormal = normal;
}