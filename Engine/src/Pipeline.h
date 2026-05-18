#pragma once

#include <volk.h>
#include <string>
#include <vector>
#include "GpuResources.h"

namespace ToyEngine
{
    struct PipelineConfig
    {
        VkShaderModule vertexShader;
        VkShaderModule fragmentShader;
        VkFormat colorFormat;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
        VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
        bool depthTest = true;
        bool depthWrite = true;
        bool blending = false;
        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
    };

    class Pipeline
    {
    public:
        void create(const GpuContext& ctx, const PipelineConfig& config, VkDescriptorSetLayout descriptorLayout);
        void create(const GpuContext& ctx, const PipelineConfig& config, std::vector<VkDescriptorSetLayout> descriptorLayouts);
        void destroy(const GpuContext& ctx);

        void bind(VkCommandBuffer cmd) const;

        VkPipeline getVkPipeline() const { return m_pipeline; }
        VkPipelineLayout getLayout() const { return m_layout; }

        static VkShaderModule loadShader(VkDevice device, const char* path);

    private:
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
    };
}
