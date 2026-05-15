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

layout(push_constant) uniform Constants 
{
    VertexBufferPtr vertexBufferAddress; 
} push;

layout(location = 0) out vec4 outColor;

void main()
{
	// Get the vertex data
    //Vertex vertex = vertices[gl_VertexIndex];
    Vertex vertex = push.vertexBufferAddress.vertices[gl_VertexIndex];
    
    vec3 position = vec3(vertex.vx, vertex.vy, vertex.vz);
    vec3 normal = vec3(vertex.nx, vertex.ny, vertex.nz);
    vec2 texCoord = vec2(vertex.tu, vertex.tv);

    gl_Position = vec4(position + vec3(0,0,0.5), 1.0);
    outColor = vec4(normal * 0.5 + vec3(0.5), 1.0);

}