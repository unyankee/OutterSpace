#include "queue_manager.h"
#include "engine.h"

QueueManager::QueueManager(Engine& engine) : m_engine(engine)
{
}

QueueData QueueManager::create_new_queue(const QUEUE_TYPE queue_type)
{
    // assert(m_queues.size() < m_engine.m_device.m_queue_family_properties[m_queue_id].queueCount - 1);

    const uint32_t queue_id = map_queue_type_id(queue_type);

    QueueData queue_data;
    queue_data.m_queue_idx  = m_queues[queue_id].size(); // wich position on its queue this belongs to
    queue_data.m_queue_type = queue_type;

    const uint32_t queue_familiy_id            = m_engine.m_device.get_queue_family_index((VkQueueFlagBits)queue_type);
    const uint32_t last_inserted_to_this_queue = m_queues[queue_id].size();

    m_queues[queue_id].push_back(VkQueue());
    vkGetDeviceQueue(m_engine.m_device.m_vulkan_device, queue_familiy_id, last_inserted_to_this_queue, &m_queues[queue_id].back());

    return queue_data;
}

VkQueue QueueManager::get_queue(const QueueData queue_data)
{
    // assert(index < m_queues.size());
    const uint32_t queue_id = map_queue_type_id(queue_data.m_queue_type);
    return m_queues[queue_id][queue_data.m_queue_idx];
}

CommandPoolData QueueManager::create_command_pool(const QUEUE_TYPE queue_type, const VkCommandPoolCreateFlagBits create_flagbits)
{
    const uint32_t queue_familiy_id = m_engine.m_device.get_queue_family_index((VkQueueFlagBits)queue_type);
    const uint32_t queue_id         = map_queue_type_id(queue_type);

    CommandPoolData command_pool_data;
    command_pool_data.m_command_pool_idx = m_command_pools[queue_id].size();
    command_pool_data.m_queue_type       = queue_type;

    m_command_pools[queue_id].push_back(VkCommandPool());

    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex        = queue_familiy_id;
    cmdPoolInfo.flags                   = create_flagbits;
    vkCreateCommandPool(m_engine.m_device.m_vulkan_device, &cmdPoolInfo, nullptr, &m_command_pools[queue_id].back());

    return command_pool_data;
}

VkCommandPool QueueManager::get_command_pool(const CommandPoolData command_pool_data)
{
    // assert(index < m_command_pools.size());
    const uint32_t queue_id = map_queue_type_id(command_pool_data.m_queue_type);
    return m_command_pools[queue_id][command_pool_data.m_command_pool_idx];
}

CommandBufferData QueueManager::create_command_buffer(const uint32_t count, VkCommandBufferLevel level, const CommandPoolData command_pool_data)
{
    const uint32_t queue_familiy_id = m_engine.m_device.get_queue_family_index((VkQueueFlagBits)command_pool_data.m_queue_type);
    const uint32_t queue_id         = map_queue_type_id(command_pool_data.m_queue_type);

    CommandBufferData command_buffer_data;
    command_buffer_data.m_command_buffer_idx = m_command_buffers.size();
    command_buffer_data.m_command_pool_data  = command_pool_data;

    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool        = m_command_pools[queue_id][command_pool_data.m_command_pool_idx];
    command_buffer_allocate_info.level              = level;
    command_buffer_allocate_info.commandBufferCount = count;

    m_command_buffers.emplace_back(std::move(CommandBuffer(count, command_buffer_data)));
    vkAllocateCommandBuffers(m_engine.m_device.m_vulkan_device, &command_buffer_allocate_info, m_command_buffers.back().get_command_buffer().data());

    return command_buffer_data;
}

CommandBuffer& QueueManager::get_command_buffer(CommandBufferData& command_buffer)
{
    return m_command_buffers[command_buffer.m_command_buffer_idx];
}

void QueueManager::free_command_buffer(CommandBuffer& command_buffer_data)
{
    // assert(&command_buffer.m_queue == this, "Trying to free a command buffer from a different queue/command pool that
    // was created from");
    const uint32_t queue_familiy_id = m_engine.m_device.get_queue_family_index((VkQueueFlagBits)command_buffer_data.m_command_buffer_data.m_command_pool_data.m_queue_type);
    const uint32_t queue_id         = map_queue_type_id(command_buffer_data.m_command_buffer_data.m_command_pool_data.m_queue_type);

    vkFreeCommandBuffers(
        m_engine.m_device.m_vulkan_device, m_command_pools[queue_id][command_buffer_data.m_command_buffer_data.m_command_pool_data.m_command_pool_idx],
        command_buffer_data.m_command_buffers.size(), command_buffer_data.m_command_buffers.data());

    command_buffer_data.m_command_buffers.clear();
}

uint32_t QueueManager::map_queue_type_id(const QUEUE_TYPE queue_type)
{
    switch (queue_type)
    {
        case QUEUE_TYPE::GRAPHICS: return 0;
        case QUEUE_TYPE::COMPUTE: return 1;
        case QUEUE_TYPE::TRANFER: return 2;
        case QUEUE_TYPE::SPARSE_BINDING: return 3;
        case QUEUE_TYPE::PROTECTED: return 4;
        default: assert("Invalid QUEUE_TYPE requested"); return 0;
    }
}

void CommandBuffer::begin_command_buffer(const uint32_t idx, const VkCommandBufferUsageFlagBits usage)
{
    if (!m_command_buffer_begun[idx])
    {
        VkCommandBufferBeginInfo command_buffer_begin_info = {};
        command_buffer_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_begin_info.flags                    = usage;
        command_buffer_begin_info.pNext                    = nullptr;

        vkBeginCommandBuffer(m_command_buffers[idx], &command_buffer_begin_info);

        m_command_buffer_begun[idx] = true;
    }
}

void CommandBuffer::end_command_buffer(const uint32_t idx)
{
    if (m_command_buffer_begun[idx])
    {
        vkEndCommandBuffer(m_command_buffers[idx]);

        m_command_buffer_begun[idx] = false;
    }
}

void CommandBuffer::reset_command_buffer(const uint32_t idx, VkCommandBufferResetFlags reset_flags)
{
    vkResetCommandBuffer(m_command_buffers[idx], reset_flags);
}

CommandBuffer::~CommandBuffer()
{
    // assert(m_command_buffers.size() == 0, "This command buffer has not being freed, but is being destroyed");
}
