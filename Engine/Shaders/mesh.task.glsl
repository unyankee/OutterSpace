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

    // get transform
    TransformData transform = TransformDataPtr(push.TransformDataAddress).transforms[push.TransformIndex];


    bool visible = false;
    if (meshletIndex < push.meshletCount)
    {
        Meshlet meshlet = MeshletBufferPtr(push.meshletBufferAddress).meshlets[meshletIndex];
        vec3 worldApex = vec3(transform.modelMatrix * vec4(meshlet.coneApexCutoff.xyz, 1.0));
        vec3 worldAxis = normalize(mat3(transform.modelMatrix) * meshlet.coneAxis.xyz);
        vec4 worldConeApexCutoff = vec4(worldApex, meshlet.coneApexCutoff.w);

        visible = !coneCull(worldConeApexCutoff, worldAxis,
        CameraBufferPtr(push.cameraBufferAddress).camera.eyePos);
    }

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
