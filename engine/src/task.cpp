#include "task.h"
#include "engine.h"

void Task::set_resource(const Parameter& parameter, std::shared_ptr<Resource> resource)
{
    // Assert if this parameter was already set
    TaskResourceConnection task_resource_connection;

    task_resource_connection.m_resource = resource;
    task_resource_connection.m_source   = resource->m_alias_id;

    switch (resource->get_resource_type())
    {
        // case ResourceType::TaskDescription: break;
        case ResourceType::ColourAttachment: m_fragment_output[parameter] = task_resource_connection; break;
        case ResourceType::DepthAttachment: m_depth_output = {parameter, task_resource_connection}; break;
        case ResourceType::UBO: m_uniform_buffers[parameter] = task_resource_connection; break;
        case ResourceType::SSBO: m_storage_buffers[parameter] = task_resource_connection; break;
        case ResourceType::VertexBuffer: m_vertex_buffers[parameter] = task_resource_connection; break;
        case ResourceType::IndexBuffer: m_index_buffers[parameter] = task_resource_connection; break;
        case ResourceType::Texture: m_textures[parameter] = task_resource_connection; break;
        case ResourceType::StorageTexture: m_storage_textures[parameter] = task_resource_connection; break;
        case ResourceType::Sampler: m_samplers[parameter] = task_resource_connection; break;
        // case ResourceType::Count: m_fragment_output[parameter] = task_resource_connection; break;
        default: break;
    }

    // m_task_resources[(uint32_t)resource->get_resource_type()][parameter] = task_resource_connection;
}

std::shared_ptr<Resource> Task::get_resource(const Parameter& parameter)
{
    if (m_vertex_buffers.find(parameter) != m_vertex_buffers.end())
    {
        return m_vertex_buffers[parameter].m_resource;
    }
    else if (m_fragment_output.find(parameter) != m_fragment_output.end())
    {
        return m_fragment_output[parameter].m_resource;
    }
    else if (m_storage_textures.find(parameter) != m_storage_textures.end())
    {
        return m_storage_textures[parameter].m_resource;
    }
    else if (m_textures.find(parameter) != m_textures.end())
    {
        return m_textures[parameter].m_resource;
    }
    else if (m_samplers.find(parameter) != m_samplers.end())
    {
        return m_samplers[parameter].m_resource;
    }
    else if (m_uniform_buffers.find(parameter) != m_uniform_buffers.end())
    {
        return m_uniform_buffers[parameter].m_resource;
    }
    else if (m_storage_buffers.find(parameter) != m_storage_buffers.end())
    {
        return m_storage_buffers[parameter].m_resource;
    }
    else if (m_index_buffers.find(parameter) != m_index_buffers.end())
    {
        return m_index_buffers[parameter].m_resource;
    }
    else if (std::get<Parameter>(m_depth_output) == parameter)
    {
        return std::get<TaskResourceConnection>(m_depth_output).m_resource;
    };
    assert("MISSING!!");
}

void Task::run(const VkCommandBuffer& command_buffer)
{
    // first check if its resources are created
    const VkDevice&        device           = m_graph.m_engine.m_device.m_vulkan_device;
    const TaskDescription& task_description = m_graph.get_task_description(m_task_description_id);

    {
        m_graph.m_pipeline_manager.update_pipeline(command_buffer, m_task_id);
        m_graph.m_pipeline_manager.update_descriptor_sets(command_buffer, m_task_id);

        {
            // transition colour + depth to ***
            // https://github.com/SaschaWillems/Vulkan/blob/313ac10de4a765997ddf5202c599e4a0ca32c8ca/examples/dynamicrendering/dynamicrendering.cpp
            // std::vector<VkImageMemoryBarrier2KHR> barriers;
            // VkImageMemoryBarrier2KHR              memoryBarrier = {};
            // memoryBarrier.sType                                 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
            // memoryBarrier.pNext;
            // memoryBarrier.srcAccessMask    = 0;
            // memoryBarrier.dstAccessMask    = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            // memoryBarrier.srcStageMask     = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            // memoryBarrier.dstStageMask     = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            // memoryBarrier.image            = m_graph.m_resources.const_get_colour_attachment(*m_fragment_output[Parameter("outFragColor")].m_resource).image();
            // memoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            // memoryBarrier.newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            // memoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            // memoryBarrier.srcQueueFamilyIndex;
            // memoryBarrier.dstQueueFamilyIndex;
            //
            // barriers.push_back(memoryBarrier);
            // memoryBarrier.srcAccessMask    = 0;
            // memoryBarrier.dstAccessMask    = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            // memoryBarrier.srcStageMask     = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            // memoryBarrier.dstStageMask     = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            // memoryBarrier.image            = m_graph.m_resources.const_get_depth_attachment(*std::get<TaskResourceConnection>(m_depth_output).m_resource).image();
            // memoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
            // memoryBarrier.newLayout        = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            // memoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1};
            // memoryBarrier.srcQueueFamilyIndex;
            // memoryBarrier.dstQueueFamilyIndex;
            //
            // barriers.push_back(memoryBarrier);
            //
            // VkDependencyInfoKHR dependencyInfo     = {};
            // dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
            // dependencyInfo.pImageMemoryBarriers    = barriers.data();
            // dependencyInfo.imageMemoryBarrierCount = barriers.size();
            //
            // m_graph.m_engine.vkCmdPipelineBarrier2KHR(command_buffer, &dependencyInfo);
        }

        {
            m_graph.begin_rendering_task(*this, task_description, command_buffer);

            // move this out, since it does not make sense to be here like this
            {
                // Update dynamic viewport state
                VkViewport viewport = {};
                viewport.height     = (float)m_graph.m_engine.m_swap_chain.height();
                viewport.width      = (float)m_graph.m_engine.m_swap_chain.width();
                viewport.minDepth   = (float)0.0f;
                viewport.maxDepth   = (float)1.0f;
                vkCmdSetViewport(command_buffer, 0, 1, &viewport);

                // Update dynamic scissor state
                VkRect2D scissor      = {};
                scissor.extent.width  = m_graph.m_engine.m_swap_chain.width();
                scissor.extent.height = m_graph.m_engine.m_swap_chain.height();
                scissor.offset.x      = 0;
                scissor.offset.y      = 0;
                vkCmdSetScissor(command_buffer, 0, 1, &scissor);
            }

            VkDeviceSize offsets[1] = {0};
            // derefence vertex/index buffer from the task
            const VertexBuffer& v_buffer = m_graph.m_resources.const_get_vertex_buffer(*m_vertex_buffers[vertex_buffer_id].m_resource);
            const IndexBuffer&  i_buffer = m_graph.m_resources.const_get_index_buffer(*m_index_buffers[index_buffer_id].m_resource);

            vkCmdBindVertexBuffers(command_buffer, 0, 1, &v_buffer.get_buffer(), offsets);

            vkCmdBindIndexBuffer(command_buffer, i_buffer.get_buffer(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(
                command_buffer, i_buffer.get_index_count(), get_instance_count(),
                /*m_mesh_system.m_meshes[i].m_index_offset,	*/ 0,
                /*m_mesh_system.m_meshes[i].m_vertex_offset,*/ 0, 0);

            m_graph.end_rendering_task(command_buffer);
        }
    }
}
