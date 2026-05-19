#include "GpuResources.h"
#include "Common/Common.h"
#include <cstring>
#include <stdexcept>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include <extern/stb/stb_image.h>

namespace ToyEngine
{

    uint32_t GpuContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (m_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("failed to find suitable memory type!");
    }

    void Buffer::create(const GpuContext& ctx, uint32_t size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties, const void* initialData)
    {
        m_size = size;

        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = size;
        bufferInfo.usage = usage;

        if (initialData && !(properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        {
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateBuffer(ctx.m_device, &bufferInfo, nullptr, &m_buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(ctx.m_device, m_buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ctx.findMemoryType(memRequirements.memoryTypeBits, properties);

        VkMemoryAllocateFlagsInfo allocFlagsInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
        if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            allocInfo.pNext = &allocFlagsInfo;
        }

        VK_CHECK(vkAllocateMemory(ctx.m_device, &allocInfo, nullptr, &m_memory));
        VK_CHECK(vkBindBufferMemory(ctx.m_device, m_buffer, m_memory, 0));

        if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        {
            VkBufferDeviceAddressInfo addressInfo{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
            addressInfo.buffer = m_buffer;
            m_gpuAddress = vkGetBufferDeviceAddress(ctx.m_device, &addressInfo);
        }

        if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            VK_CHECK(vkMapMemory(ctx.m_device, m_memory, 0, size, 0, &m_data));
            if (initialData)
            {
                memcpy(m_data, initialData, size);
            }
        }
        else if (initialData)
        {
            Buffer staging;
            staging.create(ctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, initialData);

            VkCommandBufferAllocateInfo cbAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cbAllocInfo.commandPool = ctx.m_commandPool;
            cbAllocInfo.commandBufferCount = 1;

            VkCommandBuffer cmd;
            vkAllocateCommandBuffers(ctx.m_device, &cbAllocInfo, &cmd);

            VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cmd, &beginInfo);

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(cmd, staging.m_buffer, m_buffer, 1, &copyRegion);

            vkEndCommandBuffer(cmd);

            VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmd;

            vkQueueSubmit(ctx.m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(ctx.m_graphicsQueue);

            vkFreeCommandBuffers(ctx.m_device, ctx.m_commandPool, 1, &cmd);
            staging.destroy(ctx);
        }
    }

    void Buffer::destroy(const GpuContext& ctx)
    {
        if (m_data)
        {
            vkUnmapMemory(ctx.m_device, m_memory);
            m_data = nullptr;
        }

        if (m_buffer)
        {
            vkDestroyBuffer(ctx.m_device, m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
        }

        if (m_memory)
        {
            vkFreeMemory(ctx.m_device, m_memory, nullptr);
            m_memory = VK_NULL_HANDLE;
        }
    }

    void* Buffer::map(const GpuContext& ctx)
    {
        if (!m_data)
        {
            VK_CHECK(vkMapMemory(ctx.m_device, m_memory, 0, m_size, 0, &m_data));
        }

        return m_data;
    }

    void Buffer::unmap(const GpuContext& ctx)
    {
        if (m_data)
        {
            vkUnmapMemory(ctx.m_device, m_memory);
            m_data = nullptr;
        }
    }

    void Buffer::copyDataToBuffer(const void* data, uint32_t size)
    {
        if (m_data && data)
        {
            memcpy(m_data, data, size);
        }
    }

    void Texture::load(const GpuContext& ctx, const char* path)
    {
        std::string fullPath = std::string(ENGINE_PROJECT_ROOT) + "/" + path;

        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            throw std::runtime_error("failed to load texture image!");
        }

        m_width = static_cast<uint32_t>(texWidth);
        m_height = static_cast<uint32_t>(texHeight);

        uint32_t imageSize = m_width * m_height * 4;

        Buffer staging;
        staging.create(ctx, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pixels);

        stbi_image_free(pixels);

        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_width;
        imageInfo.extent.height = m_height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateImage(ctx.m_device, &imageInfo, nullptr, &m_image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(ctx.m_device, m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ctx.findMemoryType(memRequirements.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(ctx.m_device, &allocInfo, nullptr, &m_memory));
        vkBindImageMemory(ctx.m_device, m_image, m_memory, 0);

        // Transition and copy
        VkCommandBufferAllocateInfo cbAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbAllocInfo.commandPool = ctx.m_commandPool;
        cbAllocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(ctx.m_device, &cbAllocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {m_width, m_height, 1};
        vkCmdCopyBufferToImage(cmd, staging.m_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &barrier);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        vkQueueSubmit(ctx.m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(ctx.m_graphicsQueue);

        vkFreeCommandBuffers(ctx.m_device, ctx.m_commandPool, 1, &cmd);
        staging.destroy(ctx);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(ctx.m_device, &viewInfo, nullptr, &m_view));
    }

    void Texture::create(const GpuContext& ctx, uint32_t width, uint32_t height, VkFormat format,
                         VkImageUsageFlags usage, VkImageAspectFlags aspect)
    {
        m_width = width;
        m_height = height;

        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_width;
        imageInfo.extent.height = m_height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VK_CHECK(vkCreateImage(ctx.m_device, &imageInfo, nullptr, &m_image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(ctx.m_device, m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ctx.findMemoryType(memRequirements.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(ctx.m_device, &allocInfo, nullptr, &m_memory));
        vkBindImageMemory(ctx.m_device, m_image, m_memory, 0);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(ctx.m_device, &viewInfo, nullptr, &m_view));
    }

    void Texture::uploadData(const GpuContext& ctx, const void* data, uint32_t size)
    {
        Buffer staging;
        staging.create(ctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, data);

        // Transition and copy
        VkCommandBufferAllocateInfo cbAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbAllocInfo.commandPool = ctx.m_commandPool;
        cbAllocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(ctx.m_device, &cbAllocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {m_width, m_height, 1};

        vkCmdCopyBufferToImage(cmd, staging.m_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &barrier);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        vkQueueSubmit(ctx.m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(ctx.m_graphicsQueue);

        vkFreeCommandBuffers(ctx.m_device, ctx.m_commandPool, 1, &cmd);
        staging.destroy(ctx);
    }

    void Texture::destroy(const GpuContext& ctx)
    {
        if (m_view)
        {
            vkDestroyImageView(ctx.m_device, m_view, nullptr);
        }

        if (m_image)
        {
            vkDestroyImage(ctx.m_device, m_image, nullptr);
        }

        if (m_memory)
        {
            vkFreeMemory(ctx.m_device, m_memory, nullptr);
        }

        m_image = VK_NULL_HANDLE;
        m_view = VK_NULL_HANDLE;
        m_memory = VK_NULL_HANDLE;
    }

    void RenderTarget::create(const GpuContext& ctx, uint32_t width, uint32_t height, VkFormat format,
                              VkImageUsageFlags usage, VkImageAspectFlags aspect)
    {
        m_width = width;
        m_height = height;
        m_format = format;

        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_width;
        imageInfo.extent.height = m_height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = m_format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateImage(ctx.m_device, &imageInfo, nullptr, &m_image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(ctx.m_device, m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = ctx.findMemoryType(memRequirements.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(ctx.m_device, &allocInfo, nullptr, &m_memory));
        vkBindImageMemory(ctx.m_device, m_image, m_memory, 0);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_format;
        viewInfo.subresourceRange.aspectMask = aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(ctx.m_device, &viewInfo, nullptr, &m_view));
    }

    void RenderTarget::destroy(const GpuContext& ctx)
    {
        if (m_view)
        {
            vkDestroyImageView(ctx.m_device, m_view, nullptr);
        }

        if (m_image)
        {
            vkDestroyImage(ctx.m_device, m_image, nullptr);
        }

        if (m_memory)
        {
            vkFreeMemory(ctx.m_device, m_memory, nullptr);
        }

        m_image = VK_NULL_HANDLE;
        m_view = VK_NULL_HANDLE;
        m_memory = VK_NULL_HANDLE;
    }

}
