#pragma once

#include <buffers.h>
#include <parameter.h>
#include <queue_manager.h>

#include <cassert>
#include <unordered_map>

// TMP default vertex description
struct Vertex
{
    float position[3];
    float normal[3];
};

class VertexDefinition
{
  public:
    std::vector<VkVertexInputAttributeDescription> m_input_descriptions;
    std::vector<VkVertexInputBindingDescription>   m_bindings;
    uint32_t                                       m_vertex_count;
    // The only purpose of this is to know the size of the used buffer
    // VertexStruct;
};

struct ShaderConfig
{
  public:
    ShaderConfig(const char* entry_point, const char* path, VkShaderStageFlagBits stage) : m_entry_point(entry_point), m_path(path), m_stage(stage)
    {
    }
    std::string           m_entry_point;
    std::string           m_path;
    VkShaderStageFlagBits m_stage;
};

class PipelineStepDescription
{
  public:
    PipelineStepDescription(const QUEUE_TYPE type) : m_type(type)
    {
    }

  protected:
    PipelineStepDescription() : m_type(QUEUE_TYPE::GRAPHICS){};
    const QUEUE_TYPE m_type;
};

// The idea is to be able to overwrite a set of values that describes a single pipeline 'step'
// and if is not being set, it will query the default value
// This is meant to represent only the data structure, not pointing directly to the
// data in memory, just its layout
class TaskDescription : public PipelineStepDescription
{
    friend class Task;
    friend class Graph;
    static const uint32_t NON_SET_ATTRIBUTE = -1;

  public:
    TaskDescription(uint32_t id) : PipelineStepDescription(QUEUE_TYPE::GRAPHICS), m_id(id)
    {
        m_input_assembly = get_default_input_assembly_description();
        m_rasterizer     = get_default_rasterizer_description();
        m_colour_blend   = get_default_colour_blend_description();
        m_vertex_input   = get_default_vertex_input_description();
        m_multisample    = get_default_multisample_description();
        m_viewport       = get_default_viewport_description();
        m_depth_stencil  = get_default_depth_stencil_description();
        m_dynamic        = get_default_dynamic_description();
    };

    static VkPipelineInputAssemblyStateCreateInfo get_default_input_assembly_description();
    static VkPipelineRasterizationStateCreateInfo get_default_rasterizer_description();
    static VkPipelineColorBlendStateCreateInfo    get_default_colour_blend_description();
    static VkPipelineVertexInputStateCreateInfo   get_default_vertex_input_description();
    static VkPipelineMultisampleStateCreateInfo   get_default_multisample_description();
    static VkPipelineViewportStateCreateInfo      get_default_viewport_description();
    static VkPipelineDepthStencilStateCreateInfo  get_default_depth_stencil_description();
    static VkPipelineDynamicStateCreateInfo       get_default_dynamic_description();

    void set_input_assembly_description(VkPipelineInputAssemblyStateCreateInfo description_state)
    {
        m_input_assembly = description_state;
    };
    void set_rasterizer_description(VkPipelineRasterizationStateCreateInfo description_state)
    {
        m_rasterizer = description_state;
    };
    void set_colour_blend_description(VkPipelineColorBlendStateCreateInfo description_state)
    {
        m_colour_blend = description_state;
    };
    void set_vertex_input_description(VkPipelineVertexInputStateCreateInfo description_state)
    {
        m_vertex_input = description_state;
    };
    void set_multisample_description(VkPipelineMultisampleStateCreateInfo description_state)
    {
        m_multisample = description_state;
    };
    void set_viewport_description(VkPipelineViewportStateCreateInfo description_state)
    {
        m_viewport = description_state;
    };
    void set_depth_stencil_description(VkPipelineDepthStencilStateCreateInfo description_state)
    {
        m_depth_stencil = description_state;
    };
    void set_dynamic_description(VkPipelineDynamicStateCreateInfo description_state)
    {
        m_dynamic = description_state;
    };

    const VkPipelineInputAssemblyStateCreateInfo& get_input_assembly() const
    {
        return m_input_assembly;
    };
    const VkPipelineRasterizationStateCreateInfo& get_rasterizer() const
    {
        return m_rasterizer;
    };
    const VkPipelineColorBlendStateCreateInfo& get_colour_blend() const
    {
        return m_colour_blend;
    };
    const VkPipelineVertexInputStateCreateInfo& get_vertex_input() const
    {
        return m_vertex_input;
    };
    const VkPipelineMultisampleStateCreateInfo& get_multisample() const
    {
        return m_multisample;
    };
    const VkPipelineViewportStateCreateInfo& get_viewport() const
    {
        return m_viewport;
    };
    const VkPipelineDepthStencilStateCreateInfo& get_depth_stencil() const
    {
        return m_depth_stencil;
    };
    const VkPipelineDynamicStateCreateInfo& get_dynamic() const
    {
        return m_dynamic;
    };

    const std::unordered_map<Parameter, VertexBufferReflection>& get_vertex_buffers() const
    {
        return m_vertex_buffers;
    };
    const std::unordered_map<Parameter, FragmentOutput>& get_fragment_output() const
    {
        return m_fragment_output;
    };
    const std::unordered_map<Parameter, TextureReflection>& get_storage_textures() const
    {
        return m_storage_textures;
    };
    const std::unordered_map<Parameter, TextureReflection>& get_textures() const
    {
        return m_textures;
    };
    const std::unordered_map<Parameter, SamplerReflection>& get_samplers() const
    {
        return m_samplers;
    };
    const std::tuple<Parameter, FragmentOutput>& get_depth_output() const
    {
        return m_depth_output;
    };
    const std::unordered_map<Parameter, UniformBuffer>& get_uniform_buffers() const
    {
        return m_uniform_buffers;
    };
    const std::unordered_map<Parameter, StorageBuffer>& get_storage_buffers() const
    {
        return m_storage_buffers;
    };
    const uint32_t get_task_description_id() const
    {
        return m_id;
    }

    const std::vector<VkPipelineShaderStageCreateInfo>& get_shader_stages() const
    {
        return m_shader_stages;
    }
    const VkAccessFlags2KHR        get_needed_resource_memory_access(const Parameter& parameter) const;
    const VkPipelineStageFlags2KHR get_resource_stage(const Parameter& parameter) const;

  private:
    const uint32_t m_id;

    // actual states used when creating the pipeline
    VkPipelineInputAssemblyStateCreateInfo       m_input_assembly;
    VkPipelineRasterizationStateCreateInfo       m_rasterizer;
    VkPipelineColorBlendStateCreateInfo          m_colour_blend;
    VkPipelineVertexInputStateCreateInfo         m_vertex_input;
    VkPipelineMultisampleStateCreateInfo         m_multisample;
    VkPipelineViewportStateCreateInfo            m_viewport;
    VkPipelineDepthStencilStateCreateInfo        m_depth_stencil;
    VkPipelineDynamicStateCreateInfo             m_dynamic;
    std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages;

    std::unordered_map<Parameter, VertexBufferReflection> m_vertex_buffers;
    std::unordered_map<Parameter, FragmentOutput>         m_fragment_output;
    std::unordered_map<Parameter, TextureReflection>      m_storage_textures;
    std::unordered_map<Parameter, TextureReflection>      m_textures;
    std::unordered_map<Parameter, SamplerReflection>      m_samplers;
    std::tuple<Parameter, FragmentOutput>                 m_depth_output;
    std::unordered_map<Parameter, UniformBuffer>          m_uniform_buffers;
    std::unordered_map<Parameter, StorageBuffer>          m_storage_buffers;
};
