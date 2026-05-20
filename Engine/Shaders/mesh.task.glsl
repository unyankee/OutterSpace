#version 460
#extension GL_EXT_mesh_shader : require
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"

taskPayloadSharedEXT struct TaskPayload
{
    uint meshletIndex;
} payload;

layout(local_size_x = 1) in;

void main()
{
    payload.meshletIndex = gl_WorkGroupID.x;
    EmitMeshTasksEXT(1, 1, 1);
}
