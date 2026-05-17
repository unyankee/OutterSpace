#include "PipelineManager.h"
#include "Common/Common.h"
#include <iostream>

namespace ToyEngine
{
    //PipelineManager::PipelineManager(VkDevice device) : m_device(device) {
    //}
    //
    //PipelineManager::~PipelineManager() {
    //    cleanup();
    //}

    void PipelineManager::init(VkDevice device)
    {
        std::cout << "[INFO] PipelineManager initializing with device: " << device << std::endl;
        m_device = device;
        assert(m_device != VK_NULL_HANDLE);
        std::cout << "[INFO] PipelineManager initialized successfully." << std::endl;
    }

    void PipelineManager::cleanup()
    {
        // Cleanup logic here
    }


    VkPipelineLayout PipelineManager::CreateDefaultPipelineLayout()
    {
        assert(m_device != VK_NULL_HANDLE);

        // Need to specify that the shader will receive a pointer to the vertex buffer via push constant
        // This needs to be expanded when more buffers are needed (eg: textures)
        VkPushConstantRange PushConstantRange{};
        PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; 
        PushConstantRange.offset = 0;
        PushConstantRange.size = sizeof(VkDeviceAddress) * 2; // Vertex Buffer Address + Camera Buffer Address

        VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };


        // isn't it lovely to avoid this? 
        PipelineLayoutCreateInfo.setLayoutCount = 0;
        PipelineLayoutCreateInfo.pSetLayouts = nullptr;

        PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstantRange;

        VkPipelineLayout PipelineLayout;
        VK_CHECK(vkCreatePipelineLayout(m_device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));

        return PipelineLayout;
    }


    // Want to be able to create Pipelines, this time, only creating opaque and basic one, so
    // only shaders might be different
    void PipelineManager::createMeshPipeline(VkShaderModule vs, VkShaderModule fs, VkFormat colorFormat)
    {
        assert(m_device != VK_NULL_HANDLE);
        assert(vs != VK_NULL_HANDLE);
        assert(fs != VK_NULL_HANDLE);

        PipelineDefinition NewPipelineDef;
        NewPipelineDef.PipelineLayout = CreateDefaultPipelineLayout();

        VkGraphicsPipelineCreateInfo PipelineCreateIndo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};

        VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
        ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        ShaderStages[0].module = vs;
        ShaderStages[0].pName = "main";

        ShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        ShaderStages[1].module = fs;
        ShaderStages[1].pName = "main";
        // 
        PipelineCreateIndo.stageCount = ARRAY_SIZE(ShaderStages);
        PipelineCreateIndo.pStages = ShaderStages;

        VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        };

        PipelineCreateIndo.pVertexInputState = &VertexInputStateCreateInfo;
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
        };
        InputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PipelineCreateIndo.pInputAssemblyState = &InputAssemblyCreateInfo;

        VkPipelineTessellationStateCreateInfo TessellationStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
        };
        PipelineCreateIndo.pTessellationState = &TessellationStateCreateInfo;

        VkPipelineViewportStateCreateInfo ViewportStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
        };
        ViewportStateCreateInfo.viewportCount = 1;
        ViewportStateCreateInfo.scissorCount = 1;
        PipelineCreateIndo.pViewportState = &ViewportStateCreateInfo;

        VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
        };
        RasterizationStateCreateInfo.lineWidth = 1.0f;
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        PipelineCreateIndo.pRasterizationState = &RasterizationStateCreateInfo;

        VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
        };
        MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        PipelineCreateIndo.pMultisampleState = &MultisampleStateCreateInfo;

        VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
        };
        PipelineCreateIndo.pDepthStencilState = &DepthStencilStateCreateInfo;

        VkPipelineColorBlendAttachmentState ColorBlendAttachmentState = {};
        ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
        };
        ColorBlendStateCreateInfo.attachmentCount = 1;
        ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;
        PipelineCreateIndo.pColorBlendState = &ColorBlendStateCreateInfo;

        VkDynamicState DynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo = {
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
        };
        DynamicStateCreateInfo.dynamicStateCount = ARRAY_SIZE(DynamicStates);
        DynamicStateCreateInfo.pDynamicStates = DynamicStates;
        PipelineCreateIndo.pDynamicState = &DynamicStateCreateInfo;

        PipelineCreateIndo.layout = NewPipelineDef.PipelineLayout;

        VkPipelineRenderingCreateInfo PipelineRenderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
        PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        //PipelineRenderingCreateInfo.stencilAttachmentFormat = ;

        PipelineCreateIndo.pNext = &PipelineRenderingCreateInfo;

        // This should be used later,
        // as usual now, taking shortcucts
        VkPipelineCache pipelineCache = VK_NULL_HANDLE;
        VK_CHECK(
            vkCreateGraphicsPipelines(m_device, pipelineCache, 1, &PipelineCreateIndo, nullptr, &NewPipelineDef.Pipeline
            ))

        TmpPipeline = NewPipelineDef;
    }
}
