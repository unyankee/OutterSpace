#include "graph.h"
#include "engine.h"

std::shared_ptr<Resource> GraphResourceManager::create_generic_buffer_resource(const ResourceType resource_type, std::vector<Buffer>& target_container, BufferInfo& buffer_info)
{
    auto&& new_buffer = std::make_shared<Resource>();
    new_buffer->set_resource_type(resource_type);

    Buffer buffer(buffer_info);
    buffer.create(m_engine.m_device.m_vulkan_device);

    new_buffer->m_resource_id = target_container.size();
    target_container.push_back(buffer);

    return new_buffer;
}

std::shared_ptr<Resource> GraphResourceManager::create_vertex_buffer(std::vector<Vertex>& vertex_data)
{
    uint32_t vertexBufferSize = sizeof(Vertex) * vertex_data.size();

    BufferInfo buffer_info;
    buffer_info.buffer_usage      = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    buffer_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    buffer_info.size              = vertexBufferSize;

    std::shared_ptr<Resource> vertex_buffer = std::make_shared<Resource>();

    vertex_buffer->set_resource_type(ResourceType::VertexBuffer);
    {
        VertexBuffer buffer(buffer_info, vertex_data.size());
        buffer.create(m_engine.m_device.m_vulkan_device);

        vertex_buffer->m_resource_id = m_vertex_buffers.size();
        m_vertex_buffers.push_back(buffer);
    }

    // TODO: data driven?
    VertexBuffer& buffer = get_vertex_buffer(*vertex_buffer);
    buffer.add_input_description({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
    buffer.add_input_description({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
    buffer.add_binding_description({/*binding*/ 0, /*stride*/ 0, VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX});

    BufferInfo staging_buffer_info;
    staging_buffer_info.buffer_usage      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staging_buffer_info.size              = vertexBufferSize;

    Buffer vertex_staging(staging_buffer_info);
    vertex_staging.create(m_engine.m_device.m_vulkan_device);

    vertex_staging.map(m_engine.m_device.m_vulkan_device);
    memcpy(vertex_staging.get_mapped_memory(), vertex_data.data(), vertexBufferSize);
    vertex_staging.unmap(m_engine.m_device.m_vulkan_device);

    vertex_staging.transfer_to_inmediate(m_engine, buffer, 0, 0, vertexBufferSize);

    vertex_staging.destroy_resources(m_engine.m_device.m_vulkan_device);

    return vertex_buffer;
}

std::shared_ptr<Resource> GraphResourceManager::create_vertex_buffer(VertexDefinition& vertex_definition, void* vertex_data, uint32_t data_size, uint32_t data_count)
{
    BufferInfo buffer_info;
    buffer_info.buffer_usage      = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    buffer_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    buffer_info.size              = data_size;

    std::shared_ptr<Resource> vertex_buffer = std::make_shared<Resource>();

    vertex_buffer->set_resource_type(ResourceType::VertexBuffer);
    {
        VertexBuffer buffer(buffer_info, data_count);
        buffer.create(m_engine.m_device.m_vulkan_device);

        vertex_buffer->m_resource_id = m_vertex_buffers.size();
        m_vertex_buffers.push_back(buffer);
    }

    // TODO: data driven?
    VertexBuffer& buffer = get_vertex_buffer(*vertex_buffer);
    for (auto&& binding : vertex_definition.m_bindings)
    {
        buffer.add_binding_description({binding.binding, binding.stride, binding.inputRate});
    }
    for (auto&& input_description : vertex_definition.m_input_descriptions)
    {
        buffer.add_input_description({input_description.location, input_description.binding, input_description.format, input_description.offset});
    }

    BufferInfo staging_buffer_info;
    staging_buffer_info.buffer_usage      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staging_buffer_info.size              = data_size;

    Buffer vertex_staging(staging_buffer_info);
    vertex_staging.create(m_engine.m_device.m_vulkan_device);

    vertex_staging.map(m_engine.m_device.m_vulkan_device);
    memcpy(vertex_staging.get_mapped_memory(), vertex_data, data_size);
    vertex_staging.unmap(m_engine.m_device.m_vulkan_device);

    vertex_staging.transfer_to_inmediate(m_engine, buffer, 0, 0, data_size);

    vertex_staging.destroy_resources(m_engine.m_device.m_vulkan_device);

    return vertex_buffer;
}

std::shared_ptr<Resource> GraphResourceManager::create_index_buffer(std::vector<uint32_t>& index_data)
{
    uint32_t indexBufferSize = sizeof(uint32_t) * index_data.size();

    BufferInfo buffer_info;
    buffer_info.buffer_usage      = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    buffer_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    buffer_info.size              = indexBufferSize;

    std::shared_ptr<Resource> index_buffer = std::make_shared<Resource>();

    index_buffer->set_resource_type(ResourceType::IndexBuffer);
    {
        IndexBuffer buffer(buffer_info, index_data.size());
        buffer.create(m_engine.m_device.m_vulkan_device);

        index_buffer->m_resource_id = m_index_buffers.size();
        m_index_buffers.push_back(buffer);
    }
    IndexBuffer& buffer = get_index_buffer(*index_buffer);

    BufferInfo staging_buffer_info;
    staging_buffer_info.buffer_usage      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staging_buffer_info.size              = indexBufferSize;

    Buffer index_staging(staging_buffer_info);
    index_staging.create(m_engine.m_device.m_vulkan_device);

    index_staging.map(m_engine.m_device.m_vulkan_device);
    memcpy(index_staging.get_mapped_memory(), index_data.data(), indexBufferSize);
    index_staging.unmap(m_engine.m_device.m_vulkan_device);

    index_staging.transfer_to_inmediate(m_engine, buffer, 0, 0, indexBufferSize);

    index_staging.destroy_resources(m_engine.m_device.m_vulkan_device);

    return index_buffer;
}

std::shared_ptr<Resource> GraphResourceManager::create_index_buffer(void* index_data, uint32_t data_size, uint32_t data_count)
{
    BufferInfo buffer_info;
    buffer_info.buffer_usage      = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    buffer_info.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    buffer_info.size              = data_size;

    std::shared_ptr<Resource> index_buffer = std::make_shared<Resource>();

    index_buffer->set_resource_type(ResourceType::IndexBuffer);
    {
        IndexBuffer buffer(buffer_info, data_count);
        buffer.create(m_engine.m_device.m_vulkan_device);

        index_buffer->m_resource_id = m_index_buffers.size();
        m_index_buffers.push_back(buffer);
    }
    IndexBuffer& buffer = get_index_buffer(*index_buffer);

    BufferInfo staging_buffer_info;
    staging_buffer_info.buffer_usage      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    staging_buffer_info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    staging_buffer_info.size              = data_size;

    Buffer index_staging(staging_buffer_info);
    index_staging.create(m_engine.m_device.m_vulkan_device);

    index_staging.map(m_engine.m_device.m_vulkan_device);
    memcpy(index_staging.get_mapped_memory(), index_data, data_size);
    index_staging.unmap(m_engine.m_device.m_vulkan_device);

    index_staging.transfer_to_inmediate(m_engine, buffer, 0, 0, data_size);

    index_staging.destroy_resources(m_engine.m_device.m_vulkan_device);

    return index_buffer;
}

std::shared_ptr<Resource> GraphResourceManager::create_ssbo(BufferInfo& buffer_info)
{
    buffer_info.buffer_usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    return create_generic_buffer_resource(ResourceType::SSBO, m_SSBOs, buffer_info);
}

std::shared_ptr<Resource> GraphResourceManager::create_ubo(BufferInfo& buffer_info)
{
    buffer_info.buffer_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    return create_generic_buffer_resource(ResourceType::UBO, m_UBOs, buffer_info);
}

std::shared_ptr<Resource> GraphResourceManager::create_colour_attachment(TextureInfo& attachment_info)
{
    // First sanity check if the resource is already created, if so, assert !
    auto&& new_colour_attachment = std::make_shared<Resource>();
    new_colour_attachment->set_resource_type(ResourceType::ColourAttachment);

    ColourAttachment render_texture(attachment_info);
    render_texture.create(m_engine);

    new_colour_attachment->m_resource_id = m_colour_attachments.size();
    m_colour_attachments.push_back(render_texture);

    return new_colour_attachment;
}

std::shared_ptr<Resource> GraphResourceManager::create_depth_attachment(TextureInfo& attachment_info)
{
    // First sanity check if the resource is already created, if so, assert !
    auto&& new_depth_attachment = std::make_shared<Resource>();
    new_depth_attachment->set_resource_type(ResourceType::DepthAttachment);

    ColourAttachment render_texture(attachment_info);
    render_texture.create(m_engine);

    new_depth_attachment->m_resource_id = m_depth_attachments.size();
    m_depth_attachments.push_back(render_texture);

    return new_depth_attachment;
}

// std::shared_ptr<Resource> GraphResourceManager::create_backbuffer_attachment(const std::string& resource_name, const
// TextureInfo& attachment_info)
//{
//	// First sanity check if the resource is already created, if so, assert !
//	assert(m_colour_attachment_handles.find(resource_name) == end(m_colour_attachment_handles));
//
//	auto&& new_colour_attachment = std::make_shared<Resource>();
//	new_colour_attachment->set_name(resource_name);
//	new_colour_attachment->set_resource_type(ResourceType::BackBuffer);
//
//	m_colour_attachment_handles[resource_name] = std::make_tuple(attachment_info, new_colour_attachment);
//
//	return new_colour_attachment;
// }

// void GraphResourceManager::create_resource(const std::shared_ptr<Resource> resource)
//{
//	switch (resource->get_resource_type())
//	{
//	case ResourceType::ColourAttachment:
//	{
//		m_colour_attachments.push_back(ColourAttachment(resource->get_name()));
//
//		break;
//	}
//	case ResourceType::SSBO:
//	{
//		m_SSBOs.push_back(Buffer(resource->get_name()));
//
//		break;
//	}
//	case ResourceType::UBO:
//	{
//		m_UBOs.push_back(Buffer(resource->get_name()));
//
//		break;
//	}
//	default:
//		break;
//	}
// }

void GraphResourceManager::update_UBO(const std::shared_ptr<Resource> resource, const void* data, const uint32_t size_in_bytes)
{
    Buffer& buffer = get_UBO(*resource);
    buffer.map(m_engine.m_device.m_vulkan_device);
    memcpy(buffer.get_mapped_memory(), data, size_in_bytes);
    buffer.unmap(m_engine.m_device.m_vulkan_device);
}

void GraphResourceManager::update_SSBO(const std::shared_ptr<Resource> resource, const void* data, const uint32_t size_in_bytes)
{
    Buffer& buffer = get_SSBO(*resource);
    buffer.map(m_engine.m_device.m_vulkan_device);
    memcpy(buffer.get_mapped_memory(), data, size_in_bytes);
    buffer.unmap(m_engine.m_device.m_vulkan_device);
}

ColourAttachment& GraphResourceManager::get_depth_attachment(const Resource& resource)
{
    assert(resource.get_resource_type() == ResourceType::DepthAttachment);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_depth_attachments[resource.m_resource_id];
}

ColourAttachment& GraphResourceManager::get_colour_attachment(const Resource& resource)
{
    assert(resource.get_resource_type() == ResourceType::ColourAttachment);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_colour_attachments[resource.m_resource_id];
}

Buffer& GraphResourceManager::get_UBO(const Resource& resource)
{
    assert(resource.get_resource_type() == ResourceType::UBO);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_UBOs[resource.m_resource_id];
}

Buffer& GraphResourceManager::get_SSBO(const Resource& resource)
{
    assert(resource.get_resource_type() == ResourceType::SSBO);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_SSBOs[resource.m_resource_id];
}

VertexBuffer& GraphResourceManager::get_vertex_buffer(const Resource& resource)
{
    assert(resource.get_resource_type() == ResourceType::VertexBuffer);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_vertex_buffers[resource.m_resource_id];
}

IndexBuffer& GraphResourceManager::get_index_buffer(const Resource& resource)
{
    assert(resource.get_resource_type() == ResourceType::IndexBuffer);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_index_buffers[resource.m_resource_id];
}

const ColourAttachment& GraphResourceManager::const_get_depth_attachment(const Resource& resource) const
{
    assert(resource.get_resource_type() == ResourceType::DepthAttachment);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_depth_attachments[resource.m_resource_id];
}

const ColourAttachment& GraphResourceManager::const_get_colour_attachment(const Resource& resource) const
{
    assert(resource.get_resource_type() == ResourceType::ColourAttachment);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_colour_attachments[resource.m_resource_id];
}

const Buffer& GraphResourceManager::const_get_UBO(const Resource& resource) const
{
    assert(resource.get_resource_type() == ResourceType::UBO);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_UBOs[resource.m_resource_id];
}

const Buffer& GraphResourceManager::const_get_SSBO(const Resource& resource) const
{
    assert(resource.get_resource_type() == ResourceType::SSBO);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_SSBOs[resource.m_resource_id];
}

const VertexBuffer& GraphResourceManager::const_get_vertex_buffer(const Resource& resource) const
{
    assert(resource.get_resource_type() == ResourceType::VertexBuffer);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_vertex_buffers[resource.m_resource_id];
}

const IndexBuffer& GraphResourceManager::const_get_index_buffer(const Resource& resource) const
{
    assert(resource.get_resource_type() == ResourceType::IndexBuffer);

    // resource requested, but does not exist yet, so, it needs to be created now
    return m_index_buffers[resource.m_resource_id];
}

void Graph::perform_shader_reflection(
    const VkShaderStageFlagBits shader_stage, spirv_cross::CompilerReflection& compiler_reflection, std::unordered_map<Parameter, ReflectedResource>& expected_resource)
{
    auto&& shader_resources = compiler_reflection.get_shader_resources();
    // https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide

    auto&& reflect_shader =
        [&](spirv_cross::SmallVector<spirv_cross::Resource>& buffer_block, ResourceType resource_type, const bool buffer_block_flags /*, const bool render_target*/)
    {
        for (auto&& resource : buffer_block)
        {
            ReflectedResource reflected_resource;

            auto&& location       = compiler_reflection.get_decoration(resource.id, spv::DecorationLocation);
            auto&& binding        = compiler_reflection.get_decoration(resource.id, spv::DecorationBinding);
            auto&& descriptor_set = compiler_reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);

            // SPV seems to only add this decoration to the members of an struct, if it is an struct
            // seems to be already requested, but in this version is not... so, I have to do this check.
            // also, glslang, is not adding the required decoration, so I need to swap to glslc
            bool read_only  = false;
            bool write_only = false;

            auto&& buffer_type = compiler_reflection.get_type(resource.base_type_id);

            spirv_cross::Bitset buffer_flags;
            if (buffer_block_flags)
            {
                buffer_flags = compiler_reflection.get_buffer_block_flags(resource.id);
            }
            else
            {
                buffer_flags = compiler_reflection.get_decoration_bitset(resource.id);
            }

            if (buffer_flags.get(spv::DecorationNonReadable) || ResourceType::ColourAttachment == resource_type)
            {
                write_only = true;
            }
            // UBO's are always read only, but somehow, spv does not add any extra decoration to them
            if (buffer_flags.get(spv::DecorationNonWritable) || resource_type == ResourceType::UBO)
            {
                read_only = true;
            }
            // TODO: this should produce the format used by the engine.. float32, rgba32 whatever
            // isntead of saving cols/rows and data type
            if (buffer_type.basetype == spirv_cross::SPIRType::Struct)
            {
                reflected_resource.m_members_type_data.resize(buffer_type.member_types.size());

                unsigned member_count = buffer_type.member_types.size();
                for (unsigned i = 0; i < member_count; i++)
                {
                    auto&  member_type = compiler_reflection.get_type(buffer_type.member_types[i]);
                    size_t member_size = compiler_reflection.get_declared_struct_member_size(buffer_type, i);

                    reflected_resource.m_members_type_data[i].m_member_size = member_size;
                    reflected_resource.m_members_type_data[i].m_rows        = member_type.columns; // the other way around
                    reflected_resource.m_members_type_data[i].m_cols        = member_type.vecsize;
                    reflected_resource.m_members_type_data[i].m_value_type  = (ReflectedResource::ValueType)member_type.basetype;

                    const std::string& name                          = compiler_reflection.get_member_name(buffer_type.self, i);
                    reflected_resource.m_members_type_data[i].m_name = name;
                }

                auto&&            name         = compiler_reflection.get_name(resource.base_type_id);
                const std::string resouce_name = (name.empty()) ? resource.name : name;

                reflected_resource.m_location       = location;
                reflected_resource.m_resource_type  = resource_type;
                reflected_resource.m_binding        = binding;
                reflected_resource.m_descriptor_set = descriptor_set;
                reflected_resource.m_resource_name  = resouce_name;

                reflected_resource.m_read_only             = read_only;
                reflected_resource.m_write_only            = write_only;
                expected_resource[Parameter(resouce_name)] = (reflected_resource);
            }
            else
            {
                reflected_resource.m_members_type_data.resize(buffer_type.member_types.size());

                size_t member_size = buffer_type.vecsize;
                reflected_resource.m_members_type_data.resize(1);

                reflected_resource.m_members_type_data[0].m_member_size = member_size;
                reflected_resource.m_members_type_data[0].m_rows        = buffer_type.columns; // the other way around
                reflected_resource.m_members_type_data[0].m_cols        = buffer_type.vecsize;
                reflected_resource.m_members_type_data[0].m_value_type  = (ReflectedResource::ValueType)buffer_type.basetype;
                reflected_resource.m_members_type_data[0].m_name        = resource.name;

                auto&&            name         = compiler_reflection.get_name(resource.base_type_id);
                const std::string resouce_name = (name.empty()) ? resource.name : name;

                reflected_resource.m_location       = location;
                reflected_resource.m_resource_type  = resource_type;
                reflected_resource.m_binding        = binding;
                reflected_resource.m_descriptor_set = descriptor_set;
                reflected_resource.m_resource_name  = resouce_name;

                reflected_resource.m_read_only             = read_only;
                reflected_resource.m_write_only            = write_only;
                expected_resource[Parameter(resouce_name)] = (reflected_resource);
            }
        }
    };

    reflect_shader(shader_resources.uniform_buffers, ResourceType::UBO, false);
    reflect_shader(shader_resources.storage_buffers, ResourceType::SSBO, true);

    if (shader_stage & VK_SHADER_STAGE_VERTEX_BIT)
    {
        reflect_shader(shader_resources.stage_inputs, ResourceType::VertexBuffer, false);
    }
    if (shader_stage & VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        reflect_shader(shader_resources.stage_outputs, ResourceType::ColourAttachment, false);
    }

    // needs to be expanded when needed...
}

std::shared_ptr<Resource> Graph::create_task_description(std::vector<ShaderConfig> shader_configs)
{
    // get the id used for the handle, and later on init this object,
    // via shader reflection using all the shaders
    const uint32_t                   new_task_description_id = m_task_descriptions.size();
    std::shared_ptr<TaskDescription> task_description        = std::make_shared<TaskDescription>(new_task_description_id);
    m_task_descriptions.push_back(task_description);

    // create the handle that points to this resource
    std::shared_ptr<Resource> task_desctiption_handle = std::make_shared<Resource>();
    task_desctiption_handle->m_resource_id            = new_task_description_id;
    task_desctiption_handle->m_resource_type          = ResourceType::TaskDescription;

    get_reflection_for_task_description(*task_description, shader_configs);
    // let the descriptor pool managet know about a task using this task description
    m_pipeline_manager.register_task_description(*task_description);

    return task_desctiption_handle;
}

std::shared_ptr<Task> Graph::create_task(std::shared_ptr<Resource> task_description_handle)
{
    assert(task_description_handle->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = task_description_handle->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];

    const uint32_t        new_task_id = m_tasks.size();
    std::shared_ptr<Task> task        = std::make_shared<Task>(*this, task_description_handle->m_resource_id, new_task_id);
    m_tasks.push_back(task);

    m_pipeline_manager.register_task(*task);

    return task;
}

void Graph::set_input_assembly_description(std::shared_ptr<class Resource> resource, VkPipelineInputAssemblyStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_input_assembly_description(description_state);
}

void Graph::set_rasterizer_description(std::shared_ptr<class Resource> resource, VkPipelineRasterizationStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_rasterizer_description(description_state);
}

void Graph::set_colour_blend_description(std::shared_ptr<class Resource> resource, VkPipelineColorBlendStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_colour_blend_description(description_state);
}

void Graph::set_vertex_input_description(std::shared_ptr<class Resource> resource, VkPipelineVertexInputStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_vertex_input_description(description_state);
}

void Graph::set_multisample_description(std::shared_ptr<class Resource> resource, VkPipelineMultisampleStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_multisample_description(description_state);
}

void Graph::set_viewport_description(std::shared_ptr<class Resource> resource, VkPipelineViewportStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_viewport_description(description_state);
}

void Graph::set_depth_stencil_description(std::shared_ptr<class Resource> resource, VkPipelineDepthStencilStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_depth_stencil_description(description_state);
}

void Graph::set_dynamic_description(std::shared_ptr<class Resource> resource, VkPipelineDynamicStateCreateInfo description_state)
{
    assert(resource->m_resource_type == ResourceType::TaskDescription);
    const uint32_t   idx              = resource->m_resource_id;
    TaskDescription& task_description = *m_task_descriptions[idx];
    task_description.set_dynamic_description(description_state);
}

void Graph::begin_rendering_task(const Task& task, const TaskDescription& task_description, const VkCommandBuffer& command_buffer)
{
    std::vector<VkRenderingAttachmentInfoKHR> colour_attachments;
    for (auto&& resource_pair : task_description.get_fragment_output())
    // for (uint32_t i = 0; i < task_description.get_fragment_output().size(); ++i)
    {
        VkRenderingAttachmentInfoKHR colour_attachment = {};
        colour_attachment.sType                        = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colour_attachment.clearValue                   = {0.0, 0.5, 1.0, 0.0};
        colour_attachment.imageView                    = m_resources.const_get_colour_attachment(*task.get_fragment_output().at(resource_pair.first).m_resource).view();
        colour_attachment.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colour_attachment.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
        colour_attachment.pNext;
        colour_attachment.resolveImageLayout;
        colour_attachment.resolveImageView;
        colour_attachment.resolveMode;
        colour_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colour_attachments.push_back(colour_attachment);
    }

    VkRenderingAttachmentInfoKHR depthStencilAttachment{};
    depthStencilAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    depthStencilAttachment.imageView =
        (task_description.get_depth_stencil().depthTestEnable) ? m_resources.const_get_depth_attachment(*task.get_depth_output().m_resource).view() : VK_NULL_HANDLE;
    depthStencilAttachment.imageLayout             = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    depthStencilAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
    depthStencilAttachment.clearValue.depthStencil = {1.0f, 0};

    VkRenderingInfoKHR rendering_info = {};
    rendering_info.sType              = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    rendering_info.pNext;
    rendering_info.flags;
    rendering_info.colorAttachmentCount = task_description.get_fragment_output().size();
    rendering_info.pColorAttachments    = colour_attachments.data();
    rendering_info.layerCount           = 1;
    rendering_info.pDepthAttachment     = &depthStencilAttachment;
    rendering_info.pStencilAttachment   = &depthStencilAttachment;
    rendering_info.renderArea           = {0, 0, m_engine.m_swap_chain.width(), m_engine.m_swap_chain.height()};
    rendering_info.viewMask;

    m_engine.vkCmdBeginRenderingKHR(command_buffer, &rendering_info);
}

void Graph::end_rendering_task(const VkCommandBuffer& command_buffer)
{
    m_engine.vkCmdEndRenderingKHR(command_buffer);
}

const glm::uvec2 Graph::get_fullscreen_resolution()
{
    return {m_engine.m_swap_chain.width(), m_engine.m_swap_chain.height()};
}

const VkFormat Graph::get_depth_format()
{
    return m_engine.m_device.m_depthFormat;
}

// split this function, is way too big
// https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide
void Graph::get_reflection_for_task_description(TaskDescription& task_description, std::vector<ShaderConfig>& shader_configs)
{
    for (ShaderConfig& shader : shader_configs)
    {
        std::vector<uint32_t> shader_code;
        m_engine.get_SPIRVShadercode(shader.m_path, shader_code, shader.m_stage);

        spirv_cross::CompilerReflection shader_reflection(shader_code);
        shader_reflection;

        // Create a new shader module that will be used for pipeline creation
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = shader_code.size() * sizeof(uint32_t);
        moduleCreateInfo.pCode    = shader_code.data();

        VkShaderModule shaderModule;
        vkCreateShaderModule(m_engine.m_device.m_vulkan_device, &moduleCreateInfo, NULL, &shaderModule);

        // for (auto&& shader_config : shader_configs)
        {
            VkPipelineShaderStageCreateInfo shader_stage_info = {};
            shader_stage_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shader_stage_info.stage                           = shader.m_stage;
            // copy this piece of memory
            // todo: remove this shitty thingy
            char* entry_point = new char[shader.m_entry_point.size() + 1];
            memcpy(entry_point, shader.m_entry_point.c_str(), shader.m_entry_point.size());
            entry_point[shader.m_entry_point.size()] = '\0';
            shader_stage_info.pName                  = entry_point; // TODO: something with this names is wrong? copy of a ptr that ends up destroyed a bit later?

            shader_stage_info.module = shaderModule;

            task_description.m_shader_stages.push_back(shader_stage_info);
        }

        auto&& shader_resources = shader_reflection.get_shader_resources();
        if (shader.m_stage == VK_SHADER_STAGE_VERTEX_BIT)
        {
            // query for any vertex input that is needed, only interested in location and name
            for (auto&& input_resource : shader_resources.stage_inputs)
            {
                uint32_t    location          = shader_reflection.get_decoration(input_resource.id, spv::DecorationLocation);
                std::string vertex_atrib_name = input_resource.name;

                VertexBufferReflection vertex_buffer;
                vertex_buffer.m_stage    = shader.m_stage;
                vertex_buffer.m_location = location;
                vertex_buffer.m_name.set_parameter_id(vertex_atrib_name);

                auto&&         buffer_type = shader_reflection.get_type(input_resource.base_type_id);
                const DataType data_type   = map_spirv_reflection_type(buffer_type, buffer_type.vecsize, buffer_type.columns);

                vertex_buffer.m_data_type = data_type;

                task_description.m_vertex_buffers.insert({vertex_buffer.m_name, vertex_buffer});
            }
        }
        {
            // query for any UBO/CB that is needed, same, interested in binding/sets and datatype/names of its members
            // this is a struct-only resource
            for (auto&& uniform_buffer : shader_resources.uniform_buffers)
            {
                // in case that the actual ubo has a name, this is an optional name set in the shader, so it might be
                // empty
                UniformBuffer reflected_uniform_buffer;
                reflected_uniform_buffer.m_stage = shader.m_stage;

                std::string ubo_name       = shader_reflection.get_name(uniform_buffer.base_type_id);
                uint32_t    binding        = shader_reflection.get_decoration(uniform_buffer.id, spv::DecorationBinding);
                uint32_t    descriptor_set = shader_reflection.get_decoration(uniform_buffer.id, spv::DecorationDescriptorSet);

                reflected_uniform_buffer.m_name.set_parameter_id(ubo_name);
                reflected_uniform_buffer.m_binding = binding;
                reflected_uniform_buffer.m_set     = descriptor_set;

                auto&&   buffer_type  = shader_reflection.get_type(uniform_buffer.base_type_id);
                uint32_t member_count = buffer_type.member_types.size();

                for (uint32_t i = 0; i < member_count; i++)
                {
                    StructMember                       struct_member;
                    const std::string&                 name        = shader_reflection.get_member_name(buffer_type.self, i);
                    const const spirv_cross::SPIRType& member_type = shader_reflection.get_type(buffer_type.member_types[i]);
                    const uint32_t                     member_size = shader_reflection.get_declared_struct_member_size(buffer_type, i);
                    const DataType                     data_type   = map_spirv_reflection_type(member_type, member_type.vecsize, member_type.columns);

                    struct_member.m_data_type = data_type;
                    struct_member.m_name.set_parameter_id(name);

                    reflected_uniform_buffer.m_struct_members.push_back(struct_member);
                }
                if (task_description.m_uniform_buffers.count(reflected_uniform_buffer.m_name) > 0)
                {
                    task_description.m_uniform_buffers[reflected_uniform_buffer.m_name].m_stage =
                        (VkShaderStageFlagBits)(task_description.m_uniform_buffers[reflected_uniform_buffer.m_name].m_stage | shader.m_stage);
                }
                else
                {
                    task_description.m_uniform_buffers.insert({reflected_uniform_buffer.m_name, reflected_uniform_buffer});
                }
            }
        }
        {
            // query for any Storage buffers / structured buffers that is needed, same, interested in binding/sets and
            // datatype/names of its members this is a struct-only resource too, and it also queries read/write
            // capabilities of the buffer
            for (auto&& storage_buffer : shader_resources.storage_buffers)
            {
                // in case that the actual ubo has a name, this is an optional name set in the shader, so it might be
                // empty
                StorageBuffer reflected_storage_buffer;
                reflected_storage_buffer.m_stage = shader.m_stage;

                std::string ssbao_name     = shader_reflection.get_name(storage_buffer.base_type_id);
                uint32_t    binding        = shader_reflection.get_decoration(storage_buffer.id, spv::DecorationBinding);
                uint32_t    descriptor_set = shader_reflection.get_decoration(storage_buffer.id, spv::DecorationDescriptorSet);

                reflected_storage_buffer.m_name.set_parameter_id(ssbao_name);
                reflected_storage_buffer.m_binding = binding;
                reflected_storage_buffer.m_set     = descriptor_set;

                spirv_cross::Bitset buffer_flags = shader_reflection.get_buffer_block_flags(storage_buffer.id);

                const bool read_only  = buffer_flags.get(spv::DecorationNonWritable);
                const bool write_only = buffer_flags.get(spv::DecorationNonReadable);

                MemoryAccess memory_access;
                if (read_only == false && write_only == false)
                {
                    memory_access = MemoryAccess::readwrite;
                }
                else if (read_only)
                {
                    memory_access = MemoryAccess::readonly;
                }
                else if (write_only)
                {
                    memory_access = MemoryAccess::writeonly;
                };

                reflected_storage_buffer.m_memory_access = memory_access;

                auto&&   buffer_type  = shader_reflection.get_type(storage_buffer.base_type_id);
                uint32_t member_count = buffer_type.member_types.size();

                for (uint32_t i = 0; i < member_count; i++)
                {
                    StructMember                       struct_member;
                    const std::string&                 name        = shader_reflection.get_member_name(buffer_type.self, i);
                    const const spirv_cross::SPIRType& member_type = shader_reflection.get_type(buffer_type.member_types[i]);
                    const uint32_t                     member_size = shader_reflection.get_declared_struct_member_size(buffer_type, i);
                    const DataType                     data_type   = map_spirv_reflection_type(member_type, member_type.vecsize, member_type.columns);

                    struct_member.m_data_type = data_type;
                    struct_member.m_name.set_parameter_id(name);

                    reflected_storage_buffer.m_struct_members.push_back(struct_member);
                }
                if (task_description.m_storage_buffers.count(reflected_storage_buffer.m_name) > 0)
                {
                    task_description.m_storage_buffers[reflected_storage_buffer.m_name].m_stage =
                        (VkShaderStageFlagBits)(task_description.m_storage_buffers[reflected_storage_buffer.m_name].m_stage | shader.m_stage);
                }
                else
                {
                    task_description.m_storage_buffers.insert({reflected_storage_buffer.m_name, reflected_storage_buffer});
                }
            }
        }

        if (shader.m_stage == VK_SHADER_STAGE_FRAGMENT_BIT)
        {
            // query for any fragment output that is needed, only interested in location and name
            for (auto&& output_resource : shader_resources.stage_outputs)
            {
                uint32_t    location        = shader_reflection.get_decoration(output_resource.id, spv::DecorationLocation);
                std::string attachment_name = output_resource.name;

                FragmentOutput fragment_output;
                fragment_output.m_stage    = shader.m_stage;
                fragment_output.m_location = location;
                fragment_output.m_name.set_parameter_id(attachment_name);

                auto&&         buffer_type  = shader_reflection.get_type(output_resource.base_type_id);
                const DataType data_type    = map_spirv_reflection_type(buffer_type, buffer_type.vecsize, buffer_type.columns);
                fragment_output.m_data_type = data_type;

                task_description.m_fragment_output.insert({fragment_output.m_name, fragment_output});
            }
        }

        // for (auto&& output_resource : shader_resources.storage_images) // rw image
        // for (auto&& output_resource : shader_resources.separate_samplers)
        for (auto&& output_resource : shader_resources.storage_images)
        {
            uint32_t    binding      = shader_reflection.get_decoration(output_resource.id, spv::DecorationBinding);
            std::string texture_name = output_resource.name;

            TextureReflection texture_info;
            texture_info.m_stage   = shader.m_stage;
            texture_info.m_binding = binding;
            texture_info.m_name.set_parameter_id(texture_name);
            texture_info.m_memory_access = MemoryAccess::readwrite; // TODO: check on this, maybe can be further expanded like ssbao?

            auto&&         buffer_type = shader_reflection.get_type(output_resource.base_type_id);
            const DataType data_type   = map_spirv_reflection_type(buffer_type, buffer_type.vecsize, buffer_type.columns);
            texture_info.m_data_type   = data_type;

            if (task_description.m_storage_textures.count(texture_info.m_name) > 0)
            {
                task_description.m_storage_textures[texture_info.m_name].m_stage =
                    (VkShaderStageFlagBits)(task_description.m_storage_textures[texture_info.m_name].m_stage | shader.m_stage);
            }
            else
            {
                task_description.m_storage_textures.insert({texture_info.m_name, texture_info});
            }
        }

        for (auto&& output_resource : shader_resources.separate_images)
        {
            uint32_t    binding      = shader_reflection.get_decoration(output_resource.id, spv::DecorationBinding);
            std::string texture_name = output_resource.name;

            TextureReflection texture_info;
            texture_info.m_stage         = shader.m_stage;
            texture_info.m_memory_access = MemoryAccess::readonly;
            texture_info.m_binding       = binding;
            texture_info.m_name.set_parameter_id(texture_name);

            auto&&         buffer_type = shader_reflection.get_type(output_resource.base_type_id);
            const DataType data_type   = map_spirv_reflection_type(buffer_type, buffer_type.vecsize, buffer_type.columns);
            texture_info.m_data_type   = data_type;

            if (task_description.m_textures.count(texture_info.m_name) > 0)
            {
                task_description.m_textures[texture_info.m_name].m_stage = (VkShaderStageFlagBits)(task_description.m_textures[texture_info.m_name].m_stage | shader.m_stage);
            }
            else
            {
                task_description.m_textures.insert({texture_info.m_name, texture_info});
            }
        }

        for (auto&& output_resource : shader_resources.separate_samplers)
        {
            uint32_t    binding      = shader_reflection.get_decoration(output_resource.id, spv::DecorationBinding);
            std::string sampler_name = output_resource.name;

            SamplerReflection sampler_info;
            sampler_info.m_stage   = shader.m_stage;
            sampler_info.m_binding = binding;
            sampler_info.m_name.set_parameter_id(sampler_name);

            auto&&         buffer_type = shader_reflection.get_type(output_resource.base_type_id);
            const DataType data_type   = map_spirv_reflection_type(buffer_type, buffer_type.vecsize, buffer_type.columns);
            sampler_info.m_data_type   = data_type;

            if (task_description.m_samplers.count(sampler_info.m_name) > 0)
            {
                task_description.m_samplers[sampler_info.m_name].m_stage = (VkShaderStageFlagBits)(task_description.m_textures[sampler_info.m_name].m_stage | shader.m_stage);
            }
            else
            {
                task_description.m_samplers.insert({sampler_info.m_name, sampler_info});
            }
        }
        // if depth testing is enabled, it needs to add the depth buffer dependency, since it is not an obligation
        // to add it in the shader, so it will not show in here
        if (task_description.get_depth_stencil().depthTestEnable)
        {
            uint32_t location = 0;

            FragmentOutput fragment_output;
            fragment_output.m_stage    = shader.m_stage;
            fragment_output.m_location = location;
            fragment_output.m_name     = depth_attachment_id;

            const DataType data_type    = DataType::float1;
            fragment_output.m_data_type = data_type;

            task_description.m_depth_output = {fragment_output.m_name, fragment_output};
        }
        else
        {
            task_description.m_depth_output;
        }
    }
}

DataType Graph::map_spirv_reflection_type(const spirv_cross::SPIRType& spv_type, uint32_t vec_size, uint32_t cols) const
{
    if (vec_size == cols)
    {
        switch (cols)
        {
            case 1: // is not a matrix
                switch (spv_type.basetype)
                {
                    case spirv_cross::SPIRType::BaseType::Boolean: return DataType::bool1;
                    case spirv_cross::SPIRType::BaseType::Int: return DataType::int1;
                    case spirv_cross::SPIRType::BaseType::UInt: return DataType::uint1;
                    case spirv_cross::SPIRType::BaseType::Float: return DataType::float1;
                    case spirv_cross::SPIRType::BaseType::Image: return DataType::texture; // TODO: now this will do it?
                    case spirv_cross::SPIRType::BaseType::Sampler: return DataType::sampler;
                    default: assert(false); break;
                }
            case 2:
                switch (spv_type.basetype)
                {
                    case spirv_cross::SPIRType::BaseType::Int: return DataType::int2x2;
                    case spirv_cross::SPIRType::BaseType::UInt: return DataType::uint2x2;
                    case spirv_cross::SPIRType::BaseType::Float: return DataType::float2x2;
                    default: assert(false); break;
                }
            case 3:
                switch (spv_type.basetype)
                {
                    case spirv_cross::SPIRType::BaseType::Int: return DataType::int3x3;
                    case spirv_cross::SPIRType::BaseType::UInt: return DataType::uint3x3;
                    case spirv_cross::SPIRType::BaseType::Float: return DataType::float3x3;
                    default: assert(false); break;
                }
            case 4:
                switch (spv_type.basetype)
                {
                    case spirv_cross::SPIRType::BaseType::Int: return DataType::int4x4;
                    case spirv_cross::SPIRType::BaseType::UInt: return DataType::uint4x4;
                    case spirv_cross::SPIRType::BaseType::Float: return DataType::float4x4;
                    default: assert(false); break;
                }
            default: assert(false); break;
        }
    }
    else
    {
        assert(cols == 1);
        switch (vec_size)
        {
            case 2:
                switch (spv_type.basetype)
                {
                    case spirv_cross::SPIRType::BaseType::Int: return DataType::int2;
                    case spirv_cross::SPIRType::BaseType::UInt: return DataType::uint2;
                    case spirv_cross::SPIRType::BaseType::Float: return DataType::float2;
                    default: assert(false); break;
                }
            case 3:
                switch (spv_type.basetype)
                {
                    case spirv_cross::SPIRType::BaseType::Int: return DataType::int3;
                    case spirv_cross::SPIRType::BaseType::UInt: return DataType::uint3;
                    case spirv_cross::SPIRType::BaseType::Float: return DataType::float3;
                    default: assert(false); break;
                }
            case 4:
                switch (spv_type.basetype)
                {
                    case spirv_cross::SPIRType::BaseType::Int: return DataType::int4;
                    case spirv_cross::SPIRType::BaseType::UInt: return DataType::uint4;
                    case spirv_cross::SPIRType::BaseType::Float: return DataType::float4;
                    default: assert(false); break;
                }
            default: assert(false); break;
        }
    }

    return DataType::bool1;
}

VkImage Graph::get_image_from_resource(const Resource& resource)
{
    ResourceType type = resource.get_resource_type();
    assert(type == ResourceType::ColourAttachment || type == ResourceType::StorageTexture || type == ResourceType::Texture);
    if (type == ResourceType::ColourAttachment)
    {
        return m_resources.get_colour_attachment(resource).m_image;
    }
    if (type == ResourceType::DepthAttachment)
    {
        return m_resources.get_depth_attachment(resource).m_image;
    }
    else if (type == ResourceType::StorageTexture)
    {
        assert(false);
    }
    else if (type == ResourceType::Texture)
    {
        assert(false);
    }
    assert(false);
    return VkImage();
}

VkBuffer Graph::get_buffer_from_resource(const Resource& resource)
{
    ResourceType type = resource.get_resource_type();
    assert(type == ResourceType::UBO || type == ResourceType::SSBO || type == ResourceType::VertexBuffer);
    if (type == ResourceType::SSBO)
    {
        return m_resources.get_SSBO(resource).get_buffer();
    }
    else if (type == ResourceType::UBO)
    {
        return m_resources.get_UBO(resource).get_buffer();
    }
    else if (type == ResourceType::VertexBuffer)
    {
        return m_resources.get_vertex_buffer(resource).get_buffer();
    }
    assert(false);

    return VkBuffer();
}

VkAccessFlagBits Graph::map_memory_transfer_to_access_flag_bits(const MemoryAccess transition)
{
    switch (transition)
    {
        case MemoryAccess ::readonly:
            {
                return VK_ACCESS_MEMORY_READ_BIT;
            }
            break;
        case MemoryAccess ::writeonly:
            {
                return VK_ACCESS_MEMORY_WRITE_BIT;
            }
            break;
        case MemoryAccess ::readwrite:
            {
                return (VkAccessFlagBits)(VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
            }
            break;
        default: break;
    }

    assert(false);
    return VkAccessFlagBits();
}

const VkImageLayout Graph::get_image_layout_from_task_description(const TaskDescription& description, const Parameter& parameter) const
{
    // Figure out where the paremeter is used, and map a layout for each case
    if (description.m_textures.count(parameter) > 0)
    {
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    else if (description.m_fragment_output.count(parameter) > 0)
    {
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else if (std::get<Parameter>(description.m_depth_output) == parameter)
    {
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    else if (description.m_storage_textures.count(parameter) > 0)
    {
        return VK_IMAGE_LAYOUT_GENERAL; // TODO??
    }
    assert("Missing stuff");
}
