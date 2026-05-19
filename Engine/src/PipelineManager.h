#pragma once

#include <volk.h>
#include <vector>

#include "GpuResources.h"

namespace ToyEngine
{

    class PipelineManager
    {
    public:
        void init(VkDevice device);

        void cleanup();

        void setupGlobalDescriptorSet();

        void AddTextureToGlobalDescriptorSet(Texture& texture);

        VkDescriptorSetLayout getGlobalDescriptorSetLayout()
        {
            return m_globalBindlessLayout;
        }

        VkDescriptorSet getGlobalDescriptorSet()
        {
            return m_globalBindlessDescriptorSet;
        }

        VkDescriptorSetLayout m_globalBindlessLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_globalBindlessDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool m_bindlessPool = VK_NULL_HANDLE;

    private:
        VkDevice m_device;
        uint32_t m_textureCount = 0;
    };

}
