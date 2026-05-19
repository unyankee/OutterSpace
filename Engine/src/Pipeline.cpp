#include "Pipeline.h"

#include "Common/Common.h"

#include <fstream>
#include <vector>

namespace ToyEngine
{

    void Pipeline::create(const GpuContext& ctx, const PipelineConfig& config, VkDescriptorSetLayout descriptorLayout)
    {
        create(ctx, config, std::vector<VkDescriptorSetLayout>{descriptorLayout});
    }

    void Pipeline::create(const GpuContext& ctx, const PipelineConfig& config, std::vector<VkDescriptorSetLayout> descriptorLayouts)
    {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = 128;

        VkPipelineLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layoutInfo.setLayoutCount = (uint32_t)descriptorLayouts.size();
        layoutInfo.pSetLayouts = descriptorLayouts.data();
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;

        VK_CHECK(vkCreatePipelineLayout(ctx.m_device, &layoutInfo, nullptr, &m_layout));

        VkPipelineShaderStageCreateInfo shaderStages[2] = {};

        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = config.m_vertexShader;
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = config.m_fragmentShader;
        shaderStages[1].pName = "main";

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

        VkVertexInputBindingDescription bindingDesc = {};
        bindingDesc.binding = 0;
        bindingDesc.stride = 20; // ImDrawVert: pos(8) + uv(8) + col(4)
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attrDesc[3] = {};

        attrDesc[0].location = 0;
        attrDesc[0].binding = 0;
        attrDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
        attrDesc[0].offset = 0;

        attrDesc[1].location = 1;
        attrDesc[1].binding = 0;
        attrDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
        attrDesc[1].offset = 8;

        attrDesc[2].location = 2;
        attrDesc[2].binding = 0;
        attrDesc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        attrDesc[2].offset = 16;

        if (config.m_blending)
        {
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
            vertexInputInfo.vertexAttributeDescriptionCount = 3;
            vertexInputInfo.pVertexAttributeDescriptions = attrDesc;
        }

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
        rasterizer.cullMode = config.m_cullMode;
        rasterizer.frontFace = config.m_frontFace;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depthStencil.depthTestEnable = config.m_depthTest ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = config.m_depthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = config.m_depthCompareOp;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        if (config.m_blending)
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
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
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
        renderingInfo.pColorAttachmentFormats = &config.m_colorFormat;
        renderingInfo.depthAttachmentFormat = config.m_depthFormat;
        pipelineInfo.pNext = &renderingInfo;

        VK_CHECK(vkCreateGraphicsPipelines(ctx.m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));
    }

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
