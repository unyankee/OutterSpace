#pragma once

#include <resource.h>
#include <task_description.h>

#include <memory>
#include <stdint.h>
#include <unordered_map>

struct DescriptorSetKey
{
    uint32_t m_task_id;
    uint32_t m_task_descrpition_id;

    const bool operator==(const DescriptorSetKey& other) const
    {
        return m_task_id == other.m_task_id && m_task_descrpition_id == other.m_task_descrpition_id;
    }
};

// make hashable
namespace std
{
template <> struct hash<DescriptorSetKey>
{
    std::size_t operator()(const DescriptorSetKey& key) const
    {
        return hash<uint32_t>()(key.m_task_descrpition_id << 16 + key.m_task_id);
    }
};
} // namespace std

// the idea of this class, is to provide a simple collection high level data for creating the real descriptor pool
// that will be used to allocate descriptor sets from
class PipelineManager
{
  public:
    PipelineManager(const class Graph& graph) : m_graph(graph){};
    // by providing a technique id using only the id, will look up for all needed resources
    // the technique needs to actually work
    void register_task_description(const TaskDescription& task_description);
    void register_task(const Task& task);

    void update();
    void update_descriptor_set_layouts();
    void update_pipeline(const VkCommandBuffer& command_buffer, const uint32_t task_id);
    void update_descriptor_sets(const VkCommandBuffer& command_buffer, const uint32_t task_id);
    void process_memory_transitions(const uint32_t task_id, const VkCommandBuffer& command_buffer);

    typedef uint32_t                                          TaskDescriptionID;
    typedef uint32_t                                          DescriptorSetCount;
    std::unordered_map<TaskDescriptionID, DescriptorSetCount> m_current_descriptor_set_count;

  private:
    struct ResourceTransition
    {
        Parameter                 m_resource_parameter;
        std::shared_ptr<Resource> m_resource;
        VkAccessFlags2KHR         m_current_state;
        VkAccessFlags2KHR         m_target_state;
        VkPipelineStageFlags2KHR  m_src_stage;
        VkPipelineStageFlags2KHR  m_dst_stage;
    };
    void process_memory_transitions_helper(const VkCommandBuffer& command_buffer, std::vector<ResourceTransition>& transitions);
    void add_descriptor_bindings(const TaskDescription& src, std::vector<VkDescriptorSetLayoutBinding>& bindings);
    void process_uniform_buffer_binding(const Parameter& parameter, const UniformBuffer& uniform_buffer, std::vector<VkDescriptorSetLayoutBinding>& collection);
    void process_storage_buffer_binding(const Parameter& parameter, const StorageBuffer& storage_buffer, std::vector<VkDescriptorSetLayoutBinding>& collection);
    void process_samplers_binding(const Parameter& parameter, const SamplerReflection& sampler, std::vector<VkDescriptorSetLayoutBinding>& collection);
    void process_textures_binding(const Parameter& parameter, const TextureReflection& texture, std::vector<VkDescriptorSetLayoutBinding>& collection);
    void process_storage_textures_binding(const Parameter& parameter, const TextureReflection& storage_texture, std::vector<VkDescriptorSetLayoutBinding>& collection);
    void create_descriptor_pool(const uint32_t task_description_id);
    void create_pipeline_layout(const uint32_t task_description_id);

  private:
    const class Graph& m_graph;

    std::unordered_map<uint32_t, std::vector<ResourceTransition>> m_required_transitions;

    typedef uint32_t TaskDescriptionID;
    // TODO: keep it simple for now, in the future need to explore about sharing descriptor set layout among different task descriptions
    std::unordered_map<TaskDescriptionID, std::vector<VkDescriptorSetLayoutBinding>> m_bindings_per_task_description;
    std::unordered_map<TaskDescriptionID, VkDescriptorSetLayout>                     m_descriptor_layouts;
    // Descriptor pools
    typedef uint32_t DescriptorSetCount;
    // std::unordered_map<TaskDescriptionID, DescriptorSetCount> m_current_descriptor_set_count;
    std::unordered_map<TaskDescriptionID, DescriptorSetCount> m_previous_descriptor_set_count;
    std::unordered_map<TaskDescriptionID, VkDescriptorPool>   m_descriptor_pools;
    // Pipelines
    std::unordered_map<TaskDescriptionID, VkPipelineCache>  m_pipeline_cache;
    std::unordered_map<TaskDescriptionID, VkPipelineLayout> m_pipeline_layout;
    std::unordered_map<TaskDescriptionID, VkPipeline>       m_pipeline;
    uint32_t                                                m_currently_bound_pipeline = -1;
    // Descriptor sets?

    std::unordered_map<DescriptorSetKey, VkDescriptorSet> m_descriptor_sets;

    // cache rendering info?
    // std::unordered_map<TaskDescriptionID, VkRenderingInfoKHR> m_cached_rendering_info;
};
