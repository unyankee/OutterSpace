#pragma once

#include <VulkanSDK/1.2.198.1/Include/vulkan/vulkan.h>
#include <array>
#include <stdint.h>
#include <vector>

// 1 to 1 mapping of VkQueueFlagBits
// if modified, upadte QUEUE::map_queue_type_id()
const uint32_t QUEUE_TYPE_COUNT = 5;
enum class QUEUE_TYPE
{
    GRAPHICS       = 1 << 0,
    COMPUTE        = 1 << 1,
    TRANFER        = 1 << 2,
    SPARSE_BINDING = 1 << 3,
    PROTECTED      = 1 << 4,
};

typedef uint32_t QUEUE_IDX;
typedef uint32_t COMMAND_POOL_IDX;
typedef uint32_t COMMAND_BUFFER_IDX;

struct QueueData
{
    QUEUE_IDX  m_queue_idx;
    QUEUE_TYPE m_queue_type;
};
struct CommandPoolData
{
    COMMAND_POOL_IDX m_command_pool_idx;
    QUEUE_TYPE       m_queue_type;
};
struct CommandBufferData
{
    COMMAND_BUFFER_IDX m_command_buffer_idx;
    CommandPoolData    m_command_pool_data;
};

class CommandBuffer
{
    friend class QueueManager;

  public:
    std::vector<VkCommandBuffer>& get_command_buffer()
    {
        return m_command_buffers;
    };
    VkCommandBuffer& get_command_buffer(const uint32_t idx)
    {
        return m_command_buffers[idx];
    };
    void begin_command_buffer(const uint32_t idx, const VkCommandBufferUsageFlagBits usage);
    void end_command_buffer(const uint32_t idx);
    void reset_command_buffer(const uint32_t idx, VkCommandBufferResetFlags reset_flags);

    CommandBuffer(const uint32_t command_buffers_size, CommandBufferData command_buffer_data)
        : m_command_buffers(command_buffers_size), m_command_buffer_begun(command_buffers_size), m_command_buffer_data(command_buffer_data)
    {
        for (auto&& begun_command_buffer : m_command_buffer_begun)
        {
            begun_command_buffer = false;
        }
    };
    ~CommandBuffer();

  private:
  private:
    // Allocated vulkan command buffers, life managed completely by m_queue
    std::vector<VkCommandBuffer> m_command_buffers;
    std::vector<bool>            m_command_buffer_begun;
    const CommandBufferData      m_command_buffer_data;
};

class QueueManager
{
  public:
    QueueManager(class Engine& engine);

    // needs to be abstracted, right now is dealing directly with the vulkan VkQueue class
    QueueData create_new_queue(const QUEUE_TYPE queue_type);
    VkQueue   get_queue(const QueueData queue_data);

    // needs to be abstracted, right now is dealing directly with the vulkan VkCommandPool class
    CommandPoolData create_command_pool(const QUEUE_TYPE queue_type, const VkCommandPoolCreateFlagBits create_flagbits);
    VkCommandPool   get_command_pool(const CommandPoolData command_pool_data);

    // needs to be abstracted, right now is dealing directly with the vulkan VkCommandBuffer class
    CommandBufferData create_command_buffer(const uint32_t count, VkCommandBufferLevel level, const CommandPoolData command_pool_data);
    CommandBuffer&    get_command_buffer(CommandBufferData& command_buffer);
    void              free_command_buffer(CommandBuffer& command_buffer);

  private:
    uint32_t map_queue_type_id(const QUEUE_TYPE queue_type);

  private:
    const class Engine& m_engine;

    std::array<std::vector<VkQueue>, QUEUE_TYPE_COUNT>       m_queues;        // all queried queues of this type
    std::array<std::vector<VkCommandPool>, QUEUE_TYPE_COUNT> m_command_pools; // all created command pool, 1 to 1 mapped to m_queues
    std::vector<CommandBuffer>                               m_command_buffers;
    // std::vector<CommandBuffer> m_command_buffer;
};
