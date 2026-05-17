#pragma once

#include "Common/Common.h"
#include <vector>

namespace ToyEngine {


    struct PipelineDefinition
    {
        VkPipeline Pipeline;
        VkPipelineLayout PipelineLayout;
        // Even though might be barely empty, renderpass is part of the concept
        // that defines a pipeline (not vulkan pipeline, but the actual "Pipeline")
        // we want to execute
    };
    
    
    class PipelineManager {
    public:
        //PipelineManager(VkDevice device);
        //~PipelineManager();

        void init(VkDevice device);
        void cleanup();

        VkPipelineLayout CreateDefaultPipelineLayout();
        void createMeshPipeline(VkShaderModule vs, VkShaderModule fs, VkFormat colorFormat);

        // remove, needs to update to dynamic so I can remove the render pass from places it should not be, like swapchain
        PipelineDefinition TmpPipeline;
    private:
        VkDevice m_device;
    };

}
