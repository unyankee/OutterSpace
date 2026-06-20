#include "Pipeline.h"

#include "Common/Common.h"

#include <fstream>
#include <vector>

namespace ToyEngine
{

    void Pipeline::create(const GpuContext& ctx, VkDescriptorSetLayout descriptorLayout, const std::vector<VkPushConstantRange>& pushConstantRanges)
    {
        create(ctx, std::vector<VkDescriptorSetLayout>{descriptorLayout}, pushConstantRanges);
    }

    void Pipeline::create(const GpuContext& ctx, std::vector<VkDescriptorSetLayout> descriptorLayouts, const std::vector<VkPushConstantRange>& pushConstantRanges)
    {
        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layoutInfo.setLayoutCount = (uint32_t)descriptorLayouts.size();
        layoutInfo.pSetLayouts = descriptorLayouts.data();
        layoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
        layoutInfo.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();

        VK_CHECK(vkCreatePipelineLayout(ctx.m_device, &layoutInfo, nullptr, &m_layout));

        VkPipelineShaderStageCreateInfo shaderStages[3] = {};
        uint32_t shaderStageCount = 0;

        if (m_config.m_useMeshShaders)
        {
            shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_TASK_BIT_EXT;
            shaderStages[shaderStageCount].module = m_config.m_taskShader;
            shaderStages[shaderStageCount].pName = "main";
            ++shaderStageCount;

            shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
            shaderStages[shaderStageCount].module = m_config.m_meshShader;
            shaderStages[shaderStageCount].pName = "main";
            ++shaderStageCount;
        }
        else
        {
            shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_VERTEX_BIT;
            shaderStages[shaderStageCount].module = m_config.m_vertexShader;
            shaderStages[shaderStageCount].pName = "main";
            ++shaderStageCount;
        }

        shaderStages[shaderStageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[shaderStageCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[shaderStageCount].module = m_config.m_fragmentShader;
        shaderStages[shaderStageCount].pName = "main";
        ++shaderStageCount;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
        };
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = m_config.m_cullMode;
        rasterizer.frontFace = m_config.m_frontFace;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depthStencil.depthTestEnable = m_config.m_depthTest ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = m_config.m_depthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = m_config.m_depthCompareOp;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        if (m_config.m_blending)
        {
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        else
        {
            colorBlendAttachment.blendEnable = VK_FALSE;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipelineInfo.stageCount = shaderStageCount;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = m_config.m_useMeshShaders ? nullptr : &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = m_config.m_useMeshShaders ? nullptr : &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_layout;
        pipelineInfo.renderPass = VK_NULL_HANDLE; // Using dynamic rendering

        VkPipelineRenderingCreateInfo renderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &m_config.m_colorFormat;
        renderingInfo.depthAttachmentFormat = m_config.m_depthFormat;
        pipelineInfo.pNext = &renderingInfo;

        VK_CHECK(vkCreateGraphicsPipelines(ctx.m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));
    }

    VkShaderStageFlags Pipeline::getPipelineStageMask( ) const
    {
        VkShaderStageFlags ShaderStageFlags = 0;
        ShaderStageFlags |= (m_config.m_fragmentShader != VK_NULL_HANDLE) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        ShaderStageFlags |= (m_config.m_vertexShader != VK_NULL_HANDLE) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
        ShaderStageFlags |= (m_config.m_meshShader != VK_NULL_HANDLE) ? VK_SHADER_STAGE_MESH_BIT_EXT : 0;
        ShaderStageFlags |= (m_config.m_taskShader != VK_NULL_HANDLE) ? VK_SHADER_STAGE_TASK_BIT_EXT : 0;
        
        return ShaderStageFlags;
    };
    
    void Pipeline::destroy(const GpuContext& ctx)
    {
        if (m_pipeline)
        {
            vkDestroyPipeline(ctx.m_device, m_pipeline, nullptr);
        }

        if (m_layout)
        {
            vkDestroyPipelineLayout(ctx.m_device, m_layout, nullptr);
        }

        m_pipeline = VK_NULL_HANDLE;
        m_layout = VK_NULL_HANDLE;
    }

    void Pipeline::bind(VkCommandBuffer cmd) const
    {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }

    VkShaderModule Pipeline::loadShader(VkDevice device, const char* path)
    {
        std::string fullPath = std::string(ENGINE_DIR) + "/" + path;
        std::ifstream file(fullPath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open shader file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

        VkShaderModule shaderModule;
        VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));

        return shaderModule;
    }

}
