#pragma once

#include <string>
#include <vector>
#include <functional>
#include <volk.h>
#include "GpuResources.h"
#include "ResourceManager.h"

namespace ToyEngine
{
    class ResourceManager;
    class PipelineManager;
    class Scene;

    enum PassType
    {
        EPassType_OpaquePass = 0,
        EPassType_ComputePass,
        EPassType_UIPass
    };

    struct PassAttachment
    {
        RenderTargetHandle handle;
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        VkClearValue clearValue;
    };

    struct PassContext
    {
        BufferHandle& CameraBuffer;
        ResourceManager& resourceManager;
        PipelineManager& pipelineManager;
        Scene& scene;
    };

    struct Pass
    {
        std::string name;
        PassType type = EPassType_OpaquePass;
        PipelineHandle pipeline;

        std::vector<TextureHandle> inputTextures;
        std::vector<BufferHandle> inputBuffers;

        std::function<void(VkCommandBuffer cmd, const Pass& pass, PassContext& ctx)> execute;
    };

    struct RenderBatch
    {
        std::string name;
        std::vector<PassAttachment> colorAttachments;
        PassAttachment depthAttachment;
        bool useDepth = false;

        std::vector<Pass> passes;
    };
}
