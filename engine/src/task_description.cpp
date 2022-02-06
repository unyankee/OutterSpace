#include "task_description.h"

VkPipelineInputAssemblyStateCreateInfo TaskDescription::get_default_input_assembly_description()
{
    const static VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = []() -> VkPipelineInputAssemblyStateCreateInfo
    {
        VkPipelineInputAssemblyStateCreateInfo to_return = {};
        to_return.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        to_return.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        return to_return;
    }();

    return inputAssemblyState;
}

VkPipelineRasterizationStateCreateInfo TaskDescription::get_default_rasterizer_description()
{
    // Rasterization state
    const static VkPipelineRasterizationStateCreateInfo rasterizationState = []() -> VkPipelineRasterizationStateCreateInfo
    {
        VkPipelineRasterizationStateCreateInfo to_return = {};
        to_return.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        to_return.polygonMode                            = VK_POLYGON_MODE_FILL;
        to_return.cullMode                               = VK_CULL_MODE_BACK_BIT;
        to_return.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        to_return.depthClampEnable                       = VK_FALSE;
        to_return.rasterizerDiscardEnable                = VK_FALSE;
        to_return.depthBiasEnable                        = VK_FALSE;
        to_return.lineWidth                              = 1.0f;
        return to_return;
    }();

    return rasterizationState;
}

VkPipelineColorBlendStateCreateInfo TaskDescription::get_default_colour_blend_description()
{
    static VkPipelineColorBlendAttachmentState blendAttachmentState = {};
    blendAttachmentState.colorWriteMask                             = 0xf;
    blendAttachmentState.blendEnable                                = VK_FALSE;

    static VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType                                      = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount                            = 1;
    colorBlendState.pAttachments                               = &blendAttachmentState;
    return colorBlendState;
}

VkPipelineVertexInputStateCreateInfo TaskDescription::get_default_vertex_input_description()
{
    static VkVertexInputBindingDescription vertexInputBinding = {};
    vertexInputBinding.binding                                = 0;
    vertexInputBinding.stride                                 = sizeof(Vertex);
    vertexInputBinding.inputRate                              = VK_VERTEX_INPUT_RATE_VERTEX;

    // TODO: Default values, expected position / normal (expecting to add UV's)
    static std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributs;
    vertexInputAttributs[0].binding  = 0;
    vertexInputAttributs[0].location = 0;
    vertexInputAttributs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributs[0].offset   = offsetof(Vertex, position);

    vertexInputAttributs[1].binding  = 0;
    vertexInputAttributs[1].location = 1;
    vertexInputAttributs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributs[1].offset   = offsetof(Vertex, normal);

    // Vertex input state used for pipeline creation
    static VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType                                       = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount               = 1;
    vertexInputState.pVertexBindingDescriptions                  = &vertexInputBinding;
    vertexInputState.vertexAttributeDescriptionCount             = 2;
    vertexInputState.pVertexAttributeDescriptions                = vertexInputAttributs.data();
    return vertexInputState;
}

VkPipelineMultisampleStateCreateInfo TaskDescription::get_default_multisample_description()
{
    static VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType                                       = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples                        = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.pSampleMask                                 = nullptr;
    return multisampleState;
}

VkPipelineViewportStateCreateInfo TaskDescription::get_default_viewport_description()
{
    static VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                            = 1;
    viewportState.scissorCount                             = 1;
    return viewportState;
}

VkPipelineDepthStencilStateCreateInfo TaskDescription::get_default_depth_stencil_description()
{
    static VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType                                        = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable                              = VK_TRUE;
    depthStencilState.depthWriteEnable                             = VK_TRUE;
    depthStencilState.depthCompareOp                               = VK_COMPARE_OP_LESS_OR_EQUAL;

    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    // stencil
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front             = {};
    depthStencilState.back              = {};
    return depthStencilState;
}

VkPipelineDynamicStateCreateInfo TaskDescription::get_default_dynamic_description()
{
    static std::array<VkDynamicState, 2> dynamicStateEnables;
    dynamicStateEnables[0] = (VK_DYNAMIC_STATE_VIEWPORT);
    dynamicStateEnables[1] = (VK_DYNAMIC_STATE_SCISSOR);

    static VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType                                   = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates                          = &dynamicStateEnables[0];
    dynamicState.dynamicStateCount                       = static_cast<uint32_t>(dynamicStateEnables.size());
    return dynamicState;
}

const VkAccessFlags2KHR TaskDescription::get_needed_resource_memory_access(const Parameter& parameter) const
{
    if (m_vertex_buffers.find(parameter) != m_vertex_buffers.end())
    {
        return VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR;
    }
    else if (m_fragment_output.find(parameter) != m_fragment_output.end())
    {
        return VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR;
    }
    else if (m_storage_textures.find(parameter) != m_storage_textures.end())
    {
        // return m_storage_textures.at(parameter).m_memory_access;
    }
    else if (m_textures.find(parameter) != m_textures.end())
    {
        // return MemoryAccess::readonly; VK_ACCESS_2_SHADER_READ_BIT_KHR??
    }
    else if (m_samplers.find(parameter) != m_samplers.end())
    {
        // return MemoryAccess::readonly;
    }
    else if (m_uniform_buffers.find(parameter) != m_uniform_buffers.end())
    {
        return VK_ACCESS_2_UNIFORM_READ_BIT_KHR;
    }
    else if (m_storage_buffers.find(parameter) != m_storage_buffers.end())
    {
        if (m_storage_buffers.at(parameter).m_memory_access == MemoryAccess::readonly)
        {
            return VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR;
        }
        else if (m_storage_buffers.at(parameter).m_memory_access == MemoryAccess::writeonly)
        {
            return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR;
        }
        else
        {
            return VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR | VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR;
        }
    }
    else if (std::get<Parameter>(m_depth_output) == parameter)
    {
        if (m_depth_stencil.depthWriteEnable)
        {
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR;
        }
        else
        {
            return VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR;
        }
    };
    assert("MISSING!!");
}

const VkPipelineStageFlags2KHR TaskDescription::get_resource_stage(const Parameter& parameter) const
{
    if (m_vertex_buffers.find(parameter) != m_vertex_buffers.end())
    {
        return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR;
    }
    else if (m_fragment_output.find(parameter) != m_fragment_output.end())
    {
        return VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
    }
    else if (m_storage_textures.find(parameter) != m_storage_textures.end())
    {
        // return m_storage_textures.at(parameter).m_memory_access;
    }
    else if (m_textures.find(parameter) != m_textures.end())
    {
        // return MemoryAccess::readonly; VK_ACCESS_2_SHADER_READ_BIT_KHR??
    }
    else if (m_samplers.find(parameter) != m_samplers.end())
    {
        // return MemoryAccess::readonly;
    }
    else if (m_uniform_buffers.find(parameter) != m_uniform_buffers.end())
    {
        return m_uniform_buffers.at(parameter).m_stage;
    }
    else if (m_storage_buffers.find(parameter) != m_storage_buffers.end())
    {
        return m_storage_buffers.at(parameter).m_stage;
    }
    else if (std::get<Parameter>(m_depth_output) == parameter)
    {
        return VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR;
    };
    assert("MISSING!!");
}
