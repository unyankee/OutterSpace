#version 460
#extension GL_EXT_mesh_shader : require
#extension GL_GOOGLE_include_directive : require
#extension GL_KHR_shader_subgroup_ballot : require

#include "common.glsl"

taskPayloadSharedEXT struct TaskPayload
{
    uint meshletIndices[32];
} payload;

layout(local_size_x = 32) in;

bool coneCull(vec4 coneApexCutoff, vec3 coneAxis, vec3 eyePos)
{
    if (coneAxis == vec3(0.0))
    {
        return false;
    }

    vec3 apex = coneApexCutoff.xyz;
    float cutoff = coneApexCutoff.w;

    vec3 v = apex - eyePos;
    float d2 = dot(v, v);
    float d = sqrt(d2);
    
    return dot(v, coneAxis) >= cutoff * d;
}

void main()
{
    uint threadId = gl_LocalInvocationID.x;
    uint meshletIndex = gl_WorkGroupID.x * gl_WorkGroupSize.x + threadId;
    
    // Check if we are within bounds of the meshlet count
    // (Assuming we might launch slightly more workgroups than needed)
    // For now, we'll assume the host sends enough meshlets. 
    // Actually, we should check a push constant for totalMeshletCount if available.
    // Since we don't have that yet, let's just do the test.
    
    bool visible = !coneCull(MeshletBufferPtr(push.meshletBufferAddress).meshlets[meshletIndex].coneApexCutoff, 
                            MeshletBufferPtr(push.meshletBufferAddress).meshlets[meshletIndex].coneAxis.xyz, 
                            CameraBufferPtr(push.cameraBufferAddress).camera.eyePos);

    uvec4 vote = subgroupBallot(visible);
    uint visibleCount = subgroupBallotInclusiveBitCount(vote);
    uint totalVisible = subgroupBallotBitCount(vote);

    if (visible)
    {
        payload.meshletIndices[visibleCount - 1] = meshletIndex;
    }

    if (threadId == 0)
    {
        EmitMeshTasksEXT(totalVisible, 1, 1);
    }
}
