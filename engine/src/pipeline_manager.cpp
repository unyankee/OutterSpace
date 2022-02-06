#include "pipeline_manager.h"
#include "engine.h"

void PipelineManager::register_task_description(const TaskDescription& task_description)
{
    const uint32_t task_description_id = task_description.get_task_description_id();
    const bool     already_created     = (m_bindings_per_task_description.find(task_description_id) != m_bindings_per_task_description.end());
    if (!already_created)
    {
        m_bindings_per_task_description[task_description_id] = std::vector<VkDescriptorSetLayoutBinding>(); // TODO: this looks dumb
    }
    std::vector<VkDescriptorSetLayoutBinding>& bindings = m_bindings_per_task_description[task_description_id];
    // bindings.m_dirty   = true;
    add_descriptor_bindings(task_description, bindings);
}

void PipelineManager::register_task(const Task& task)
{
    m_current_descriptor_set_count[task.get_task_description_id()]++;
}

void PipelineManager::update()
{
    m_currently_bound_pipeline      = -1;
    bool refresh_memory_transitions = false;
    // detect that we need to rebuild some resources, due to some new tasks/tasks description added to the graph
    for (auto&& current_descriptor : m_current_descriptor_set_count)
    {
        const uint32_t current_count  = current_descriptor.second;
        uint32_t&      previous_count = m_previous_descriptor_set_count[current_descriptor.first];
        if (current_count != previous_count)
        {
            previous_count = current_count;
            create_descriptor_pool(current_descriptor.first);
            create_pipeline_layout(current_descriptor.first);
            refresh_memory_transitions = true;
        }
    }
    if (refresh_memory_transitions)
    {
        m_required_transitions.clear();
        for (int32_t i = m_graph.m_tasks.size() - 1; i >= 0; --i)
        {
            // looping backwards, with the assumption that any resource dependency actually needs
            // any other previous task using it
            const Task& task           = m_graph.get_task(i);
            auto&&      task_resources = task.get_resources();
            for (auto&& resource_pair : task_resources)
            {
                // just check if the memory layout fo rthis and the dependency task is the same, do not go deeper
                // than that
                const TaskResourceConnection resource_connection = resource_pair.second;
                const bool                   has_dependency      = resource_connection.m_source != -1;
                if (has_dependency)
                {
                    const Task& previous_task          = m_graph.get_task(resource_connection.m_source);
                    const bool  avoid_dependency_check = task.get_task_description_id() == previous_task.get_task_description_id();
                    if (avoid_dependency_check)
                    {
                        continue;
                    }
                    const TaskDescription& task_description          = m_graph.get_task_description(task.get_task_description_id());
                    const TaskDescription& previous_task_description = m_graph.get_task_description(previous_task.get_task_description_id());
                    // get the desired memory layout for this specific resource
                    // and the needed for the previous task
                    // if they are the same, do not do anything
                    // otherwise, keep track of this needed memory transition
                    const Parameter& resource_paramenter = resource_pair.first;
                    Parameter        previous_resource_paramenter;
                    // the name of the resource on the dependency task might be different
                    {
                        for (auto&& previour_resource_pair : previous_task.get_resources())
                        {
                            if (resource_pair.second.m_resource->point_to_same_resource(*previour_resource_pair.second.m_resource))
                            {
                                previous_resource_paramenter = previour_resource_pair.first;
                            }
                        }
                    }

                    const VkAccessFlags2KHR target_access   = task_description.get_needed_resource_memory_access(resource_paramenter);
                    const VkAccessFlags2KHR previous_access = previous_task_description.get_needed_resource_memory_access(previous_resource_paramenter);

                    const VkPipelineStageFlags2KHR src_stage = previous_task_description.get_needed_resource_memory_access(previous_resource_paramenter);
                    const VkPipelineStageFlags2KHR dst_stage = previous_task_description.get_needed_resource_memory_access(previous_resource_paramenter);
                    // Slot in the task executed right before being used
                    m_required_transitions[task.get_task_id()].push_back(
                        {resource_pair.first, resource_connection.m_resource, previous_access, target_access, src_stage, dst_stage});
                }
            }
        }
        // TODO: should collapse all the memory transitions in here
        // we do not want to execute memory transitions right before using any resource
        // that does not make any sense
        // instead we want to collapse them all, on a way that we query a lot of them at once

        // loop por cada entrada
        // comprobar la entrada anterior
        // si el recurso no se necesita, comprobar hacia atras, hasta que se necesite y tenga otro memory state

        // for (uint32_t task_itetator = m_graph.m_tasks.size() - 1; task_itetator >= 0; --task_itetator)
        //{
        //     const Task&   task     = m_graph.get_task(task_itetator);
        //     const int32_t task_idx = task.get_task_id();
        //     for (int32_t resource_index = 0; resource_index < m_required_transitions[task_idx].size(); ++resource_index)
        //     {
        //         // For each resource in a task, check when is previously needed, and if the layout is different
        //         const ResourceTransition& current_transition = m_required_transitions[task_idx][resource_index];
        //         for (int32_t previous_task_itetator = task_itetator - 1; task_itetator >= 0; --task_itetator)
        //         {
        //             const Task&   previous_task     = m_graph.get_task(previous_task_itetator);
        //             const int32_t previous_task_idx = task.get_task_id();
        //             for (int32_t previous_resource_index = 0; previous_resource_index < m_required_transitions[previous_task_itetator].size(); ++previous_resource_index)
        //             {
        //                 if (current_transition.m_resource->point_to_same_resource(*m_required_transitions[previous_task_itetator][previous_resource_index].m_resource))
        //                 {
        //                     if (m_required_transitions[previous_task_itetator][previous_resource_index].m_target_state != current_transition.m_target_state)
        //                     {
        //                         // move current position to be in previous?
        //                         m_required_transitions[]
        //                     }
        //                 }
        //             }
        //         }
        //     }
        // }
    }
}

void PipelineManager::update_descriptor_set_layouts()
{
    for (auto&& bindings_per_task_description : m_bindings_per_task_description)
    {
        std::vector<VkDescriptorSetLayoutBinding>& bindings = bindings_per_task_description.second;
        // if (bindings.m_dirty) // TODO: if this is recreated, it means that also other resources making use of this guy needs to be recreated...
        {
            VkDescriptorSetLayoutCreateInfo dsl_create_info;
            dsl_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dsl_create_info.bindingCount = bindings_per_task_description.second.size();
            dsl_create_info.flags        = 0;
            dsl_create_info.pBindings    = bindings_per_task_description.second.data();
            dsl_create_info.pNext        = nullptr;

            VkDescriptorSetLayout new_layout;
            vkCreateDescriptorSetLayout(m_graph.m_engine.m_device.m_vulkan_device, &dsl_create_info, nullptr, &new_layout);
            m_descriptor_layouts[bindings_per_task_description.first] = new_layout;
        }
    }
}

void PipelineManager::process_memory_transitions_helper(const VkCommandBuffer& command_buffer, std::vector<ResourceTransition>& transitions)
{
}

void PipelineManager::add_descriptor_bindings(const TaskDescription& src, std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    // TODO:
    // let's keep it simple for now...
    for (auto&& uniform_buffer : src.get_uniform_buffers())
    {
        process_uniform_buffer_binding(uniform_buffer.first, uniform_buffer.second, bindings);
    };
    for (auto&& storage_buffer : src.get_storage_buffers())
    {
        process_storage_buffer_binding(storage_buffer.first, storage_buffer.second, bindings);
    };
    for (auto&& sampler : src.get_samplers())
    {
        process_samplers_binding(sampler.first, sampler.second, bindings);
    };
    for (auto&& textures : src.get_textures())
    {
        process_textures_binding(textures.first, textures.second, bindings);
    };
    for (auto&& storage_textures : src.get_storage_textures())
    {
        process_storage_textures_binding(storage_textures.first, storage_textures.second, bindings);
    };
    update_descriptor_set_layouts();
}

void PipelineManager::process_uniform_buffer_binding(const Parameter& parameter, const UniformBuffer& uniform_buffer, std::vector<VkDescriptorSetLayoutBinding>& collection)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding                      = uniform_buffer.m_binding;
    binding.descriptorCount              = 1; // array of shader resources?
    binding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.pImmutableSamplers           = nullptr;
    binding.stageFlags                   = uniform_buffer.m_stage;
    collection.push_back(binding);
}

void PipelineManager::process_storage_buffer_binding(const Parameter& parameter, const StorageBuffer& storage_buffer, std::vector<VkDescriptorSetLayoutBinding>& collection)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding                      = storage_buffer.m_binding;
    binding.descriptorCount              = 1;
    binding.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.pImmutableSamplers           = nullptr;
    binding.stageFlags                   = storage_buffer.m_stage;
    collection.push_back(binding);
}

void PipelineManager::process_samplers_binding(const Parameter& parameter, const SamplerReflection& sampler, std::vector<VkDescriptorSetLayoutBinding>& collection)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding                      = sampler.m_binding;
    binding.descriptorCount              = 1;
    binding.descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLER;
    binding.pImmutableSamplers           = nullptr;
    binding.stageFlags                   = sampler.m_stage;
    collection.push_back(binding);
}

void PipelineManager::process_textures_binding(const Parameter& parameter, const TextureReflection& texture, std::vector<VkDescriptorSetLayoutBinding>& collection)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding                      = texture.m_binding;
    binding.descriptorCount              = 1;
    binding.descriptorType               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    binding.pImmutableSamplers           = nullptr;
    binding.stageFlags                   = texture.m_stage;
    collection.push_back(binding);
}

void PipelineManager::process_storage_textures_binding(const Parameter& parameter, const TextureReflection& storage_texture, std::vector<VkDescriptorSetLayoutBinding>& collection)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding                      = storage_texture.m_binding;
    binding.descriptorCount              = 1;
    binding.descriptorType               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding.pImmutableSamplers           = nullptr;
    binding.stageFlags                   = storage_texture.m_stage;
    collection.push_back(binding);
}

void PipelineManager::create_descriptor_pool(const uint32_t task_description_id)
{
    const TaskDescription& task_description = m_graph.get_task_description(task_description_id);
    // rebuild the descriptor pool
    std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
    if (task_description.get_uniform_buffers().size() > 0)
    {
        VkDescriptorPoolSize descriptor_pool_size;
        descriptor_pool_size.descriptorCount = task_description.get_uniform_buffers().size();
        descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_pool_sizes.push_back(descriptor_pool_size);
    }
    if (task_description.get_storage_buffers().size() > 0)
    {
        VkDescriptorPoolSize descriptor_pool_size;
        descriptor_pool_size.descriptorCount = task_description.get_storage_buffers().size();
        descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptor_pool_sizes.push_back(descriptor_pool_size);
    }
    if (task_description.get_textures().size() > 0)
    {
        VkDescriptorPoolSize descriptor_pool_size;
        descriptor_pool_size.descriptorCount = task_description.get_textures().size();
        descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptor_pool_sizes.push_back(descriptor_pool_size);
    }
    if (task_description.get_samplers().size() > 0)
    {
        VkDescriptorPoolSize descriptor_pool_size;
        descriptor_pool_size.descriptorCount = task_description.get_samplers().size();
        descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptor_pool_sizes.push_back(descriptor_pool_size);
    }
    if (task_description.get_storage_textures().size() > 0)
    {
        VkDescriptorPoolSize descriptor_pool_size;
        descriptor_pool_size.descriptorCount = task_description.get_storage_textures().size();
        descriptor_pool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptor_pool_sizes.push_back(descriptor_pool_size);
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info;
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.flags = 0;
    // descriptor_pool_create_info.maxSets       = 1; // TODO: this should be the amount of tasks using this description?
    descriptor_pool_create_info.maxSets       = m_current_descriptor_set_count[task_description_id]; // ??
    descriptor_pool_create_info.pNext         = nullptr;
    descriptor_pool_create_info.poolSizeCount = descriptor_pool_sizes.size();
    descriptor_pool_create_info.pPoolSizes    = descriptor_pool_sizes.data();

    VkDescriptorPool pool;
    vkCreateDescriptorPool(m_graph.m_engine.m_device.m_vulkan_device, &descriptor_pool_create_info, nullptr, &pool);
    m_descriptor_pools[task_description_id] = pool;
}

void PipelineManager::create_pipeline_layout(const uint32_t task_description_id)
{
    const TaskDescription& task_description = m_graph.get_task_description(task_description_id);
    const VkDevice&        device           = m_graph.m_engine.m_device.m_vulkan_device;
    {
        VkPipelineCacheCreateInfo pipeline_cache_info = {};
        pipeline_cache_info.sType                     = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pipeline_cache_info.flags;
        pipeline_cache_info.initialDataSize;
        pipeline_cache_info.pInitialData;
        pipeline_cache_info.pNext;

        VkPipelineCache cache;
        vkCreatePipelineCache(device, &pipeline_cache_info, nullptr, &cache);
        m_pipeline_cache[task_description_id] = cache;
    }

    {
        VkPipelineLayoutCreateInfo pipeline_layout_info = {};
        pipeline_layout_info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.pNext                      = nullptr;
        pipeline_layout_info.flags                      = 0;
        pipeline_layout_info.pSetLayouts                = &m_descriptor_layouts[task_description_id];
        pipeline_layout_info.setLayoutCount             = 1;
        pipeline_layout_info.pushConstantRangeCount;
        pipeline_layout_info.pPushConstantRanges;

        VkPipelineLayout layout;
        vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &layout);
        m_pipeline_layout[task_description_id] = layout;
    }
}

void PipelineManager::update_pipeline(const VkCommandBuffer& command_buffer, const uint32_t task_id)
{
    const Task&            task                = m_graph.get_task(task_id);
    const uint32_t         task_description_id = task.get_task_description_id();
    const TaskDescription& task_description    = m_graph.get_task_description(task_description_id);
    const VkDevice&        device              = m_graph.m_engine.m_device.m_vulkan_device;

    if (m_currently_bound_pipeline == task_description_id)
    {
        return;
    }

    if (m_pipeline.find(task_description_id) == m_pipeline.end())
    {
        // get the format of the colour attachments from the registered resources
        std::vector<VkFormat> colour_attachments_format;
        for (auto&& fragment_output : task_description.get_fragment_output())
        {
            const Parameter&              fragment_output_parameter = fragment_output.first;
            const TaskResourceConnection& resource_connection       = task.get_fragment_output().at(fragment_output_parameter);
            const Resource&               resource_handle           = *resource_connection.m_resource;
            const ColourAttachment&       actual_resource           = m_graph.m_resources.const_get_colour_attachment(resource_handle);

            colour_attachments_format.push_back(actual_resource.m_render_texture_info.format);
        }

        VkFormat depth_attachment_format = {};
        {
            if (task_description.get_default_depth_stencil_description().depthTestEnable)
            {
                const Parameter&              fragment_output_parameter = std::get<Parameter>(task_description.get_depth_output());
                const TaskResourceConnection& resource_connection       = task.get_depth_output();
                const Resource&               resource_handle           = *resource_connection.m_resource;
                const ColourAttachment&       actual_resource           = m_graph.m_resources.const_get_depth_attachment(resource_handle);

                depth_attachment_format = (actual_resource.m_render_texture_info.format);
            }
        }

        VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info = {};
        pipeline_rendering_create_info.sType                            = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        pipeline_rendering_create_info.colorAttachmentCount             = task_description.get_fragment_output().size();
        pipeline_rendering_create_info.pColorAttachmentFormats          = colour_attachments_format.data();
        pipeline_rendering_create_info.depthAttachmentFormat            = depth_attachment_format;
        pipeline_rendering_create_info.pNext                            = nullptr;
        pipeline_rendering_create_info.stencilAttachmentFormat;
        pipeline_rendering_create_info.viewMask;

        VkGraphicsPipelineCreateInfo pipeline_create_info = {};
        pipeline_create_info.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_create_info.basePipelineHandle;
        pipeline_create_info.basePipelineIndex;
        pipeline_create_info.pNext               = &pipeline_rendering_create_info;
        pipeline_create_info.flags               = 0;
        pipeline_create_info.layout              = m_pipeline_layout[task_description_id];
        pipeline_create_info.pColorBlendState    = &task_description.get_colour_blend();
        pipeline_create_info.pDepthStencilState  = &task_description.get_depth_stencil();
        pipeline_create_info.pDynamicState       = &task_description.get_dynamic();
        pipeline_create_info.pInputAssemblyState = &task_description.get_input_assembly();
        pipeline_create_info.pMultisampleState   = &task_description.get_multisample();
        pipeline_create_info.pRasterizationState = &task_description.get_rasterizer();
        pipeline_create_info.pTessellationState;
        pipeline_create_info.pVertexInputState = &task_description.get_vertex_input();
        pipeline_create_info.pViewportState    = &task_description.get_viewport();
        pipeline_create_info.pStages           = task_description.get_shader_stages().data();
        pipeline_create_info.stageCount        = task_description.get_shader_stages().size();
        pipeline_create_info.renderPass        = VK_NULL_HANDLE;
        pipeline_create_info.subpass;

        VkPipeline pipeline;
        vkCreateGraphicsPipelines(device, m_pipeline_cache[task_description_id], 1, &pipeline_create_info, nullptr, &pipeline);
        m_pipeline[task_description_id] = pipeline;
    }

    m_currently_bound_pipeline = task_description_id;
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline[task_description_id]);
}

// TODO: this is the worst right now, by far...
void PipelineManager::update_descriptor_sets(const VkCommandBuffer& command_buffer, const uint32_t task_id)
{
    const Task&            task                = m_graph.get_task(task_id);
    const uint32_t         task_description_id = task.get_task_description_id();
    const TaskDescription& task_description    = m_graph.get_task_description(task_description_id);
    const VkDevice&        device              = m_graph.m_engine.m_device.m_vulkan_device;
    const DescriptorSetKey key{task_id, task_description_id};

    if (m_descriptor_sets.find(key) == m_descriptor_sets.end())
    {
        VkDescriptorSetAllocateInfo alloc_info;
        alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool     = m_descriptor_pools[task_description_id];
        alloc_info.descriptorSetCount = 1;
        alloc_info.pNext              = nullptr;
        alloc_info.pSetLayouts        = &m_descriptor_layouts[task_description_id];

        VkDescriptorSet descriptor_set;
        vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set);
        m_descriptor_sets[key] = descriptor_set;
    }

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;

    uint32_t               descriptor_sets_to_update = 0;
    const VkDescriptorSet& descriptor_set            = m_descriptor_sets[key];

    for (auto&& uniform_buffer : task_description.get_uniform_buffers())
    {
        const Parameter& uniform_parameter_name = uniform_buffer.first;
        if (uniform_parameter_name.is_bound())
        {
            continue;
        }
        // not greatly thought...
        const_cast<Parameter&>(uniform_parameter_name).set_bound();
        auto&& task_resource_handle = task.get_uniform_buffers().at(uniform_parameter_name).m_resource;

        const Buffer& ubo_buffer = m_graph.m_resources.const_get_UBO(*task_resource_handle);

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet               = descriptor_set;
        write_descriptor_set.descriptorCount      = 1;
        write_descriptor_set.descriptorType       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write_descriptor_set.dstBinding           = uniform_buffer.second.m_binding;
        write_descriptor_set.pBufferInfo          = &ubo_buffer.get_descriptor();

        write_descriptor_sets.push_back(write_descriptor_set);
        ++descriptor_sets_to_update;
    };
    for (auto&& storage_buffer : task_description.get_storage_buffers())
    {
        const Parameter& uniform_parameter_name = storage_buffer.first;
        if (uniform_parameter_name.is_bound())
        {
            continue;
        }
        // not greatly thought...
        const_cast<Parameter&>(uniform_parameter_name).set_bound();

        auto&& task_resource_handle = task.get_storage_buffers().at(uniform_parameter_name).m_resource;

        const Buffer& ssbo_buffer = m_graph.m_resources.const_get_SSBO(*task_resource_handle);

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet               = descriptor_set;
        write_descriptor_set.descriptorCount      = 1;
        write_descriptor_set.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_descriptor_set.dstBinding           = storage_buffer.second.m_binding;
        write_descriptor_set.pBufferInfo          = &ssbo_buffer.get_descriptor();

        write_descriptor_sets.push_back(write_descriptor_set);
        ++descriptor_sets_to_update;
    };

    /*
    for (auto&& storage_buffer : task_description.get_storage_textures())
    {
        const Parameter& uniform_parameter_name = storage_buffer.first;
        if (uniform_parameter_name.is_bound())
        {
            continue;
        }
        // not greatly thought...
        const_cast<Parameter&>(uniform_parameter_name).set_bound();

        auto&& task_resource_handle = task.get_resources().at(uniform_parameter_name).m_resource;
        // TODO: right now only works with colour attachments
        const ColourAttachment& texture = m_graph.m_resources.const_get_colour_attachment(*task_resource_handle);

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet               = descriptor_set;
        write_descriptor_set.descriptorCount      = 1;
        write_descriptor_set.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write_descriptor_set.dstBinding           = storage_buffer.second.m_binding;
        write_descriptor_set.pImageInfo           = &texture.m_descriptor;

        write_descriptor_sets.push_back(write_descriptor_set);
        ++descriptor_sets_to_update;
    };

    for (auto&& storage_buffer : task_description.get_textures())
    {
        const Parameter& uniform_parameter_name = storage_buffer.first;
        if (uniform_parameter_name.is_bound())
        {
            continue;
        }
        // not greatly thought...
        const_cast<Parameter&>(uniform_parameter_name).set_bound();

        auto&& task_resource_handle = task.get_resources().at(uniform_parameter_name).m_resource;
        // TODO: right now only works with colour attachments
        const ColourAttachment& texture = m_graph.m_resources.const_get_colour_attachment(*task_resource_handle);

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet               = descriptor_set;
        write_descriptor_set.descriptorCount      = 1;
        write_descriptor_set.descriptorType       = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write_descriptor_set.dstBinding           = storage_buffer.second.m_binding;

        // VkDescriptorImageInfo descriptorImageInfo{};
        // texture.m_descriptor.sampler = 0;
        // texture.m_descriptor.sampler = nullptr; // from previous version this used to be nullptr...!!??
        //  descriptorImageInfo.imageView   = texture.view();
        // texture.m_descriptor.imageView   = texture.m_depth_view_test;
        // texture.m_descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        write_descriptor_set.pImageInfo = &texture.m_descriptor;

        write_descriptor_sets.push_back(write_descriptor_set);
        ++descriptor_sets_to_update;
    };
    for (auto&& storage_buffer : task_description.get_samplers())
    {
        const Parameter& uniform_parameter_name = storage_buffer.first;
        if (uniform_parameter_name.is_bound())
        {
            continue;
        }
        // not greatly thought...
        const_cast<Parameter&>(uniform_parameter_name).set_bound();

        auto&& task_resource_handle = task.get_resources().at(uniform_parameter_name).m_resource;
        // Sampler&         sampler                = m_resources.get_sampler(task_resource_handle);

        VkWriteDescriptorSet write_descriptor_set = {};
        write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstSet               = descriptor_set;
        write_descriptor_set.descriptorCount      = 1;
        write_descriptor_set.descriptorType       = VK_DESCRIPTOR_TYPE_SAMPLER;
        write_descriptor_set.dstBinding           = storage_buffer.second.m_binding;
        // write_descriptor_set.pImageInfo           = sampler.; // TODO: WERE HERE !!

        write_descriptor_sets.push_back(write_descriptor_set);
        ++descriptor_sets_to_update;
    };
    */
    // TODO: JUST USE SIZE()!!
    if (write_descriptor_sets.size() > 0)
    {
        vkUpdateDescriptorSets(device, descriptor_sets_to_update, write_descriptor_sets.data(), 0, nullptr);
    }

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout[task_description_id], 0, 1, &m_descriptor_sets[key], 0, nullptr);
}

// TODO: you were here !
// needs to finish this memory transtition and then jump to updateing old classes (swapchain)
void PipelineManager::process_memory_transitions(const uint32_t task_id, const VkCommandBuffer& command_buffer)
{

    std::vector<VkImageMemoryBarrier2KHR>  image_memory_barriers;
    std::vector<VkMemoryBarrier2KHR>       memory_barriers;
    std::vector<VkBufferMemoryBarrier2KHR> buffer_memory_barriers;

    for (const ResourceTransition& transition : m_required_transitions[task_id])
    {
        const Task&            task             = m_graph.get_task(task_id);
        const TaskDescription& task_descrpition = m_graph.get_task_description(task.get_task_description_id());

        const ResourceType resource_type = transition.m_resource->get_resource_type();
        if (resource_type == ResourceType::ColourAttachment || resource_type == ResourceType::DepthAttachment)
        {
            ColourAttachment&        colour_attachment = (resource_type == ResourceType::ColourAttachment)
                                                             ? const_cast<Graph&>(m_graph).m_resources.get_colour_attachment(*transition.m_resource)
                                                             : const_cast<Graph&>(m_graph).m_resources.get_depth_attachment(*transition.m_resource);
            VkImageMemoryBarrier2KHR image_barrier;
            image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
            image_barrier.pNext = nullptr;

            image_barrier.srcAccessMask = transition.m_current_state;
            image_barrier.dstAccessMask = transition.m_target_state;

            image_barrier.srcStageMask = transition.m_src_stage; // VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            image_barrier.dstStageMask = transition.m_src_stage; // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            image_barrier.image        = colour_attachment.image();

            image_barrier.oldLayout = colour_attachment.m_render_texture_info.m_current_layout; // VK_IMAGE_LAYOUT_UNDEFINED;

            image_barrier.newLayout =
                m_graph.get_image_layout_from_task_description(task_descrpition, transition.m_resource_parameter); // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            image_barrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            image_barrier.srcQueueFamilyIndex;
            image_barrier.dstQueueFamilyIndex;

            if (colour_attachment.m_render_texture_info.m_current_layout != image_barrier.newLayout)
            {
                // override current with new layout
                colour_attachment.m_render_texture_info.m_current_layout = image_barrier.newLayout;
                image_memory_barriers.push_back(image_barrier);
            }
        }
        // NEEDS TO HANDLE EVERYTHING ELSE TOO !
    }

    VkDependencyInfoKHR dependencyInfo = {};
    dependencyInfo.sType               = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
    dependencyInfo.pNext               = nullptr;
    dependencyInfo.dependencyFlags;
    dependencyInfo.pImageMemoryBarriers     = image_memory_barriers.data();
    dependencyInfo.imageMemoryBarrierCount  = image_memory_barriers.size();
    dependencyInfo.pMemoryBarriers          = memory_barriers.data();
    dependencyInfo.memoryBarrierCount       = memory_barriers.size();
    dependencyInfo.pBufferMemoryBarriers    = buffer_memory_barriers.data();
    dependencyInfo.bufferMemoryBarrierCount = buffer_memory_barriers.size();

    m_graph.m_engine.vkCmdPipelineBarrier2KHR(command_buffer, &dependencyInfo);
    // process_memory_transitions_helper(command_buffer, m_required_transitions[task_id]);
}
