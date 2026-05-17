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

    void PipelineManager::AddTextureToGlobalDescriptorSet(TextureResource& texture)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture.view;
        imageInfo.sampler = nullptr; 

        uint32_t currentSlot = m_textureCount++;
        texture.bindlessIndex = currentSlot;
        
        VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        descriptorWrite.dstSet = globalBindlessDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = currentSlot; // First one, the one that should have the ++; 
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        

        
        
        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);


        // going to add a default sampler here...
        VkSampler linearSampler;
        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &linearSampler));
        
        VkDescriptorImageInfo samplerInfoWrite{};
        samplerInfoWrite.sampler = linearSampler; 

        VkWriteDescriptorSet samplerWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        samplerWrite.dstSet = globalBindlessDescriptorSet;
        samplerWrite.dstBinding = 1;              
        samplerWrite.dstArrayElement = 0;         
        samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerWrite.descriptorCount = 1;
        samplerWrite.pImageInfo = &samplerInfoWrite;

        vkUpdateDescriptorSets(m_device, 1, &samplerWrite, 0, nullptr);
    }
    
    void PipelineManager::setupGlobalDescriptorSet()
    {
        VkDescriptorPoolSize poolSizes[2] = {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 10 }
        };

        VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT; 
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;

        VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &bindlessPool));

        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = bindlessPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &globalBindlessLayout; 

        VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &globalBindlessDescriptorSet));
    }


    VkPipelineLayout PipelineManager::CreateDefaultPipelineLayout()
    {
        
        assert(m_device != VK_NULL_HANDLE);

        
        {
            // global textures 
            VkDescriptorSetLayoutBinding bindings[2] = {};
            bindings[0].binding = 0; 
            bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            bindings[0].descriptorCount = 1000; 
            bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            // global samplers
            bindings[1].binding = 1; 
            bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            bindings[1].descriptorCount = 10; 
            bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            VkDescriptorBindingFlags bindingFlags[2] = {
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
            };

            VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
            extendedInfo.bindingCount = 2;
            extendedInfo.pBindingFlags = bindingFlags;

            VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            layoutInfo.pNext = &extendedInfo;
            layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT; 
            layoutInfo.bindingCount = 2;
            layoutInfo.pBindings = bindings;
            
            VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &globalBindlessLayout));
        }        

        VkPushConstantRange PushConstantRange{};
        {
            // Need to specify that the shader will receive a pointer to the vertex buffer via push constant
            // This needs to be expanded when more buffers are needed (eg: textures)
            PushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            PushConstantRange.offset = 0;
            PushConstantRange.size = sizeof(DefaultPipelineLayout); // Vertex Buffer Address + Camera Buffer Address
        }
        
        VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        PipelineLayoutCreateInfo.setLayoutCount = 1;
        PipelineLayoutCreateInfo.pSetLayouts = &globalBindlessLayout;
        PipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        PipelineLayoutCreateInfo.pPushConstantRanges = &PushConstantRange;


        VK_CHECK(vkCreatePipelineLayout(m_device, &PipelineLayoutCreateInfo, nullptr, &PipelineLayout));

        // TODO: WARNING: This should not be here, is here only for the purpose of loading the first texture
        setupGlobalDescriptorSet();
        
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

        DepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;   
        DepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;  
        DepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS; 
        DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        DepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        
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
