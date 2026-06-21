#pragma once

#include <volk.h>
#include <string>
#include <vector>

#include "GpuResources.h"

namespace ToyEngine
{

    struct PipelineConfig
    {
        VkShaderModule m_vertexShader = VK_NULL_HANDLE;
        VkShaderModule m_taskShader = VK_NULL_HANDLE;
        VkShaderModule m_meshShader = VK_NULL_HANDLE;
        VkShaderModule m_fragmentShader = VK_NULL_HANDLE;
        VkFormat m_colorFormat;
        VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
        VkCullModeFlags m_cullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace m_frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        bool m_depthTest = true;
        bool m_depthWrite = true;
        bool m_blending = false;
        bool m_useMeshShaders = false;
        VkCompareOp m_depthCompareOp = VK_COMPARE_OP_GREATER; // Reverse depth by default
    };

    class Pipeline
    {
    public:
        Pipeline() = default;
        explicit Pipeline(const PipelineConfig& config) : m_config(config) {}
        ~Pipeline() = default;

        void create(const GpuContext& ctx, VkDescriptorSetLayout descriptorLayout, const std::vector<VkPushConstantRange>& pushConstantRanges = {});
        void create(const GpuContext& ctx, std::vector<VkDescriptorSetLayout> descriptorLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges = {});
        void destroy(const GpuContext& ctx);

        void bind(VkCommandBuffer cmd) const;

        VkPipeline getVkPipeline() const
        {
            return m_pipeline;
        }

        VkPipelineLayout getLayout() const
        {
            return m_layout;
        }

        static VkShaderModule loadShader(VkDevice device, const char* path);

        VkShaderStageFlags getPipelineStageMask( )const;
    private:
        PipelineConfig m_config{};
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
    };

}
