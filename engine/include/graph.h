#pragma once

#include <buffers_description.h>
#include <pipeline_manager.h>
#include <resource.h>
#include <shader_reflection.h>
#include <task_description.h>

#include <VulkanSDK/1.2.198.1/Include/spirv_cross/spirv_reflect.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class GraphResourceManager
{
    friend class Graph;

  public:
    GraphResourceManager(class Engine& engine) : m_engine(engine)
    {
    }

    std::shared_ptr<Resource> create_vertex_buffer(std::vector<Vertex>& vertex_data); // should be able to define the vertex format... create a class that describes this resource
    std::shared_ptr<Resource> create_vertex_buffer(VertexDefinition& vertex_definition, void* vertex_data, uint32_t data_size, uint32_t data_count);

    std::shared_ptr<Resource> create_index_buffer(std::vector<uint32_t>& index_data); // TODO: there is no need to use 32bits always...create a class that describes this resource
    std::shared_ptr<Resource> create_index_buffer(void* index_data, uint32_t data_size, uint32_t data_count);

    std::shared_ptr<Resource> create_ssbo(BufferInfo& buffer_info);
    std::shared_ptr<Resource> create_ubo(BufferInfo& buffer_info);
    std::shared_ptr<Resource> create_colour_attachment(TextureInfo& attachment_info);
    std::shared_ptr<Resource> create_depth_attachment(TextureInfo& attachment_info);

    // this one and ssbo and any 'buffer' should go through the same interface...
    void update_UBO(const std::shared_ptr<Resource> resource, const void* data, const uint32_t size_in_bytes);
    void update_SSBO(const std::shared_ptr<Resource> resource, const void* data, const uint32_t size_in_bytes); // bad...

    const ColourAttachment& const_get_depth_attachment(const Resource& resource) const;
    const ColourAttachment& const_get_colour_attachment(const Resource& resource) const;
    const Buffer&           const_get_UBO(const Resource& resource) const;
    const Buffer&           const_get_SSBO(const Resource& resource) const;
    const VertexBuffer&     const_get_vertex_buffer(const Resource& resource) const;
    const IndexBuffer&      const_get_index_buffer(const Resource& resource) const;

    ColourAttachment& get_depth_attachment(const Resource& resource);
    ColourAttachment& get_colour_attachment(const Resource& resource);
    Buffer&           get_UBO(const Resource& resource);
    Buffer&           get_SSBO(const Resource& resource);
    VertexBuffer&     get_vertex_buffer(const Resource& resource);
    IndexBuffer&      get_index_buffer(const Resource& resource);

  private:
    std::shared_ptr<Resource> create_generic_buffer_resource(const ResourceType resource_type, std::vector<Buffer>& target_container, BufferInfo& buffer_info);

  public:
    std::vector<ColourAttachment> m_colour_attachments;
    std::vector<ColourAttachment> m_depth_attachments;
    std::vector<Buffer>           m_UBOs;
    std::vector<Buffer>           m_SSBOs;
    std::vector<VertexBuffer>     m_vertex_buffers;
    std::vector<IndexBuffer>      m_index_buffers;

    //
    // std::unordered_map<std::string, std::shared_ptr<Resource>>	m_blackboard;

    // std::vector<> m_renderpasses;

    // std::unordered_map<ResourceType,
    //	std::unordered_map<std::string, std::unique_ptr<>>> m_write_resources;

  private:
  private:
    class Engine& m_engine;
};

class Graph
{
  public:
    Graph(class Engine& engine) : m_engine(engine), m_resources(engine), m_pipeline_manager(*this){};

    // create a task description out of shder files
    std::shared_ptr<class Resource> create_task_description(std::vector<ShaderConfig> shader_configs);
    std::shared_ptr<Task>           create_task(std::shared_ptr<Resource> task_description);

    void set_input_assembly_description(std::shared_ptr<class Resource> resource, VkPipelineInputAssemblyStateCreateInfo description_state);
    void set_rasterizer_description(std::shared_ptr<class Resource> resource, VkPipelineRasterizationStateCreateInfo description_state);
    void set_colour_blend_description(std::shared_ptr<class Resource> resource, VkPipelineColorBlendStateCreateInfo description_state);
    void set_vertex_input_description(std::shared_ptr<class Resource> resource, VkPipelineVertexInputStateCreateInfo description_state);
    void set_multisample_description(std::shared_ptr<class Resource> resource, VkPipelineMultisampleStateCreateInfo description_state);
    void set_viewport_description(std::shared_ptr<class Resource> resource, VkPipelineViewportStateCreateInfo description_state);
    void set_depth_stencil_description(std::shared_ptr<class Resource> resource, VkPipelineDepthStencilStateCreateInfo description_state);
    void set_dynamic_description(std::shared_ptr<class Resource> resource, VkPipelineDynamicStateCreateInfo description_state);

    void             begin_rendering_task(const Task& task, const TaskDescription& task_description, const VkCommandBuffer& command_buffer);
    void             end_rendering_task(const VkCommandBuffer& command_buffer);
    const glm::uvec2 get_fullscreen_resolution();
    const VkFormat   get_depth_format();

    const TaskDescription& get_task_description(const uint32_t task_description_id) const
    {
        return *m_task_descriptions[task_description_id];
    };
    const Task& get_task(const uint32_t task_id) const
    {
        return *m_tasks[task_id];
    };

    std::shared_ptr<Resource> task_description1;

    std::vector<class Step*> m_steps;

    std::vector<std::shared_ptr<TaskDescription>> m_task_descriptions;
    std::vector<std::shared_ptr<Task>>            m_tasks;
    GraphResourceManager                          m_resources;
    PipelineManager                               m_pipeline_manager;

    // private:
    VkAccessFlagBits    map_memory_transfer_to_access_flag_bits(const MemoryAccess transition);
    VkImage             get_image_from_resource(const Resource& resource);
    VkBuffer            get_buffer_from_resource(const Resource& resource);
    const VkImageLayout get_image_layout_from_task_description(const TaskDescription& description, const Parameter& parameter) const;

    void get_reflection_for_task_description(TaskDescription& task_description, std::vector<ShaderConfig>& shader_configs);

    // highly probably this is the wrong place, (like most of this code) just testing...
    DataType map_spirv_reflection_type(const spirv_cross::SPIRType& spv_type, uint32_t vec_size, uint32_t colummns) const;
    void     perform_shader_reflection(
            const VkShaderStageFlagBits shader_stage, spirv_cross::CompilerReflection& compiler_reflection, std::unordered_map<Parameter, ReflectedResource>& expected_resource);

  public:
    class Engine& m_engine;
};
