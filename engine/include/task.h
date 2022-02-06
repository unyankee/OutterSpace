#pragma once

#include <parameter.h>
#include <resource.h>

#include <VulkanSDK/1.2.198.1/Include/vulkan/vulkan.h>

#include <cassert>
#include <memory>
#include <stdint.h>
#include <unordered_map>

struct TaskResourceConnection
{
    std::shared_ptr<Resource> m_resource;
    uint32_t                  m_source = -1;
    // uint32_t                  m_target = -1;

    std::tuple<std::shared_ptr<Resource>, Parameter> m_source_resource_data; // fill when building the graph?
    std::tuple<std::shared_ptr<Resource>, Parameter> m_target_resource_data; // fill when building the graph?
    // uint32_t                  m_target = -1; // fill when building the graph?
    // Parameter                 m_source_parameter;
};

class Task : public ID
{
    friend class Graph;

  public:
    Task(class Graph& graph, const uint32_t task_description_id, const uint32_t task_id) : m_task_description_id(task_description_id), m_task_id(task_id), m_graph(graph){};

    void                      set_resource(const Parameter& parameter, std::shared_ptr<Resource> resource);
    std::shared_ptr<Resource> get_resource(const Parameter& parameter);

    uint32_t get_instance_count() const
    {
        return m_instance_count;
    };
    uint32_t get_vertex_offset() const
    {
        return m_vertex_offset;
    };
    void set_instance_count(uint32_t instance_count)
    {
        m_instance_count = instance_count;
    };
    void set_vertex_offset(uint32_t vertex_offset)
    {
        m_vertex_offset = vertex_offset;
    };
    // right now let's build the graph manually
    // later on, this should be called auto by the graph
    void run(const VkCommandBuffer& command_buffer);

    const uint32_t get_task_id() const
    {
        return m_task_id;
    };

    const uint32_t get_task_description_id() const
    {
        return m_task_description_id;
    };

    const std::unordered_map<Parameter, TaskResourceConnection> get_resources() const
    {
        auto&& insert_into_output = [](const std::unordered_map<Parameter, TaskResourceConnection>& container, std::unordered_map<Parameter, TaskResourceConnection>& output)
        {
            for (auto&& pair : container)
            {
                output[pair.first] = pair.second;
            }
        };
        std::unordered_map<Parameter, TaskResourceConnection> output;
        insert_into_output(m_index_buffers, output);
        insert_into_output(m_vertex_buffers, output);
        insert_into_output(m_fragment_output, output);
        insert_into_output(m_storage_textures, output);
        insert_into_output(m_textures, output);
        insert_into_output(m_samplers, output);
        insert_into_output(m_uniform_buffers, output);
        insert_into_output(m_storage_buffers, output);

        output[std::get<Parameter>(m_depth_output)] = std::get<TaskResourceConnection>(m_depth_output);

        return output;
    };

    const std::unordered_map<Parameter, TaskResourceConnection>& get_index_buffers() const
    {
        return m_index_buffers;
    }
    const std::unordered_map<Parameter, TaskResourceConnection>& get_vertex_buffers() const
    {
        return m_vertex_buffers;
    }
    const std::unordered_map<Parameter, TaskResourceConnection>& get_fragment_output() const
    {
        return m_fragment_output;
    }
    const std::unordered_map<Parameter, TaskResourceConnection>& get_storage_textures() const
    {
        return m_storage_textures;
    }
    const std::unordered_map<Parameter, TaskResourceConnection>& get_textures() const
    {
        return m_textures;
    }
    const std::unordered_map<Parameter, TaskResourceConnection>& get_samplers() const
    {
        return m_samplers;
    }
    const TaskResourceConnection& get_depth_output() const
    {
        return std::get<TaskResourceConnection>(m_depth_output);
    }
    const std::unordered_map<Parameter, TaskResourceConnection>& get_uniform_buffers() const
    {
        return m_uniform_buffers;
    }
    const std::unordered_map<Parameter, TaskResourceConnection>& get_storage_buffers() const
    {
        return m_storage_buffers;
    }

  private:
    // const Parameter find_resource_parameter(Resource& resource);
    // std::array<std::unordered_map<Parameter, TaskResourceConnection>, (size_t)ResourceType::Count> m_task_resources;

    std::unordered_map<Parameter, TaskResourceConnection> m_index_buffers;
    std::unordered_map<Parameter, TaskResourceConnection> m_vertex_buffers;
    std::unordered_map<Parameter, TaskResourceConnection> m_fragment_output;
    std::unordered_map<Parameter, TaskResourceConnection> m_storage_textures;
    std::unordered_map<Parameter, TaskResourceConnection> m_textures;
    std::unordered_map<Parameter, TaskResourceConnection> m_samplers;
    std::tuple<Parameter, TaskResourceConnection>         m_depth_output;
    std::unordered_map<Parameter, TaskResourceConnection> m_uniform_buffers;
    std::unordered_map<Parameter, TaskResourceConnection> m_storage_buffers;

    // const ID& m_task_description_id;
    uint32_t       m_instance_count = 0;
    uint32_t       m_vertex_offset  = 0;
    const uint32_t m_task_description_id;
    const uint32_t m_task_id;
    class Graph&   m_graph;
};
