#include <cassert>

#include <buffers.h>
#include <engine.h>

void Buffer::map(const VkDevice& vulkan_device)
{
    assert(m_mapped_memory == nullptr);
    // if (m_mapped_memory == nullptr)
    {
        vkMapMemory(vulkan_device, m_memory, 0, m_memory_requirements.size, 0, (void**)&m_mapped_memory);
    }
}

void Buffer::unmap(const VkDevice& vulkan_device)
{
    assert(m_mapped_memory != nullptr);
    // if (m_mapped_memory != nullptr)
    {
        vkUnmapMemory(vulkan_device, m_memory);
        m_mapped_memory = nullptr;
    }
}

void Buffer::transfer_to(Engine& engine, Buffer& target_buffer, const uint32_t src_offset, const uint32_t dst_offset, const uint32_t size)
{
    // TODO: wrong queue/command buffer in use here!!
    // copy buffer to buffer operation
    // needs to get the proper queue to do this
    // right now, it will make use of Graphics queue, for testing purposes, and avoiding having to deal
    // with barriers sync
    // it should be done in a transfer queue if possible !!
    auto&& command_bufffer = engine.m_queue_manager.get_command_buffer(engine.m_graphics_command_buffers_data);
    // VkCommandBuffer graphics_command_buffer =
    //	engine.m_queue_manager.get_command_buffer(
    //		engine.m_graphics_command_buffers_data).get_command_buffer(engine.m_current_buffers_idx);
    VkBufferCopy copyRegion;
    copyRegion.size      = size;
    copyRegion.dstOffset = dst_offset;
    copyRegion.srcOffset = src_offset;

    command_bufffer.begin_command_buffer(engine.m_current_buffers_idx, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkCmdCopyBuffer(command_bufffer.get_command_buffer(engine.m_current_buffers_idx), m_buffer, target_buffer.m_buffer, 1, &copyRegion);
}

void Buffer::transfer_to_inmediate(Engine& engine, Buffer& target_buffer, const uint32_t src_offset, const uint32_t dst_offset, const uint32_t size)
{
    auto&& command_bufffer = engine.m_queue_manager.get_command_buffer(engine.m_transfer_command_buffers_data);

    VkBufferCopy copyRegion;
    copyRegion.size      = size;
    copyRegion.dstOffset = dst_offset;
    copyRegion.srcOffset = src_offset;

    command_bufffer.begin_command_buffer(engine.m_current_buffers_idx, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkCmdCopyBuffer(command_bufffer.get_command_buffer(engine.m_current_buffers_idx), m_buffer, target_buffer.m_buffer, 1, &copyRegion);

    command_bufffer.end_command_buffer(engine.m_current_buffers_idx);

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &command_bufffer.get_command_buffer(engine.m_current_buffers_idx);

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags             = 0;
    VkFence fence;
    vkCreateFence(engine.m_device.m_vulkan_device, &fenceCreateInfo, nullptr, &fence);
    // Submit to the queue
    vkQueueSubmit(engine.m_queue_manager.get_queue(engine.m_transfer_queue), 1, &submitInfo, fence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(engine.m_device.m_vulkan_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

    vkDestroyFence(engine.m_device.m_vulkan_device, fence, nullptr);

    // engine.m_queue_manager.free_command_buffer(command_bufffer);
}

void Buffer::flush(const VkDevice& vulkan_device, const uint32_t offset, const uint32_t size)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = m_memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    vkFlushMappedMemoryRanges(vulkan_device, 1, &mappedRange);
}

void Buffer::create(const VkDevice& vulkan_device)
{
    if (get_created())
    {
        return;
    }
    // Vertex shader uniform buffer block
    VkBufferCreateInfo   bufferInfo = {};
    VkMemoryAllocateInfo allocInfo  = {};
    allocInfo.sType                 = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext                 = nullptr;

    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size  = m_buffer_info.size;
    bufferInfo.usage = m_buffer_info.buffer_usage;

    // Create a new buffer
    vkCreateBuffer(vulkan_device, &bufferInfo, nullptr, &m_buffer);
    vkGetBufferMemoryRequirements(vulkan_device, m_buffer, &m_memory_requirements);
    allocInfo.allocationSize = m_memory_requirements.size;

    // TODO: handle this engine instance access
    Engine& engine = Engine::instance();

    allocInfo.memoryTypeIndex = engine.getMemoryTypeIndex(m_memory_requirements.memoryTypeBits, m_buffer_info.memory_properties);
    vkAllocateMemory(vulkan_device, &allocInfo, nullptr, &(m_memory));
    vkBindBufferMemory(vulkan_device, m_buffer, m_memory, 0);

    m_descriptor.buffer = m_buffer;
    m_descriptor.offset = 0;
    m_descriptor.range  = m_buffer_info.size;

    set_created();
}

void Buffer::destroy_resources(const VkDevice& vulkan_device)
{
    vkDestroyBuffer(vulkan_device, m_buffer, nullptr);
    vkFreeMemory(vulkan_device, m_memory, nullptr);
    unset_created();
}

void ColourAttachment::create(const Engine& engine)
{
    if (get_created())
    {
        return;
    }

    const VkDevice& vulkan_device = engine.m_device.m_vulkan_device;

    m_extent.height = m_render_texture_info.extent.height;
    m_extent.width  = m_render_texture_info.extent.width;

    // Will create the needed image resource
    // will allocate the needed memory and bind it to the image
    // will create a image view to work with this render texture
    VkImageCreateInfo image = {};
    image.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType         = m_render_texture_info.imageType;
    image.format            = m_render_texture_info.format;
    image.extent            = m_render_texture_info.extent;
    image.mipLevels         = 1;
    image.arrayLayers       = 1;
    image.samples           = VK_SAMPLE_COUNT_1_BIT;
    image.tiling            = VK_IMAGE_TILING_OPTIMAL;
    image.usage             = m_render_texture_info.usage;
    image.initialLayout     = m_render_texture_info.m_current_layout;

    vkCreateImage(vulkan_device, &image, nullptr, &m_image);
    // -------------------------------------------------------------//
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vulkan_device, m_image, &memReqs);

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize       = memReqs.size;
    memAlloc.memoryTypeIndex      = engine.getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(vulkan_device, &memAlloc, nullptr, &m_memory);
    vkBindImageMemory(vulkan_device, m_image, m_memory, 0);

    // -------------------------------------------------------------//
    VkImageAspectFlags aspectMask = 0;
    if (m_render_texture_info.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else if (m_render_texture_info.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    VkImageViewCreateInfo image_view = {};
    image_view.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view.viewType              = VK_IMAGE_VIEW_TYPE_2D;
    image_view.format                = m_render_texture_info.format;
    image_view.subresourceRange      = {};

    image_view.subresourceRange.aspectMask     = aspectMask;
    image_view.subresourceRange.baseMipLevel   = 0;
    image_view.subresourceRange.levelCount     = 1;
    image_view.subresourceRange.baseArrayLayer = 0;
    image_view.subresourceRange.layerCount     = 1;
    image_view.image                           = m_image;

    vkCreateImageView(vulkan_device, &image_view, nullptr, &m_view);

    if (m_render_texture_info.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    {
        image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vkCreateImageView(vulkan_device, &image_view, nullptr, &m_depth_view_test);
    }

    // m_descriptor.imageLayout = ;
    m_descriptor.imageView = m_view;
    // m_descriptor.sampler = ;

    set_created();
}
