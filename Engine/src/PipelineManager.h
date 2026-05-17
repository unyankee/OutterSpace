#pragma once

#include "Common/Common.h"
#include <vector>

namespace ToyEngine {


    struct PipelineDefinition
    {
        VkPipeline Pipeline;
        VkPipelineLayout PipelineLayout;
    };
    
    
    class PipelineManager {
    public:
        //PipelineManager(VkDevice device);
        //~PipelineManager();

        void init(VkDevice device);
        void cleanup();

        void setupGlobalDescriptorSet();
        void AddTextureToGlobalDescriptorSet(TextureResource& texture);
        
        VkPipelineLayout CreateDefaultPipelineLayout();
        void createMeshPipeline(VkShaderModule vs, VkShaderModule fs, VkFormat colorFormat);

        // remove, needs to update to dynamic so I can remove the render pass from places it should not be, like swapchain
        PipelineDefinition TmpPipeline;

        // should not be here... but need it for tmp testing
        // once working will be refactored to the right new class
        VkDescriptorSetLayout globalBindlessLayout = VK_NULL_HANDLE;
        VkDescriptorSet globalBindlessDescriptorSet;
        VkDescriptorPool bindlessPool;
        VkPipelineLayout PipelineLayout;
    private:
        VkDevice m_device;
        uint32_t m_textureCount = 0;
    };

}
