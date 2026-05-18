#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_gpu_shader_int64 : require

layout(push_constant) uniform Constants 
{
    uint64_t vertexBufferAddress; 
    uint64_t cameraBufferAddress; 
    uint textureIndex;            
    uint samplerIndex;           
} push;
