#pragma once

#include <volk.h>
#include <vector>
#include "GpuResources.h"

namespace ToyEngine {

    class PipelineManager {
    public:
        void init(VkDevice device);
        void cleanup();

        void setupGlobalDescriptorSet();
        void AddTextureToGlobalDescriptorSet(Texture& texture);
        
        VkDescriptorSetLayout getGlobalDescriptorSetLayout() { return globalBindlessLayout; }
        VkDescriptorSet getGlobalDescriptorSet() { return globalBindlessDescriptorSet; }

        VkDescriptorSetLayout globalBindlessLayout = VK_NULL_HANDLE;
        VkDescriptorSet globalBindlessDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool bindlessPool = VK_NULL_HANDLE;

    private:
        VkDevice m_device;
        uint32_t m_textureCount = 0;
    };

}
