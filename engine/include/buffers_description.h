#pragma once

#include <VulkanSDK/1.2.198.1/Include/vulkan/vulkan.h>

struct TextureInfo
{
    VkImageType           imageType;
    VkFormat              format;
    VkExtent3D            extent;
    VkSampleCountFlagBits samples;
    VkImageUsageFlags     usage;
    VkImageLayout         m_current_layout;
    // VkImageLayout         finalLayout;

    VkAttachmentLoadOp  loadOp;
    VkAttachmentStoreOp storeOp;
    VkAttachmentLoadOp  stencilLoadOp;
    VkAttachmentStoreOp stencilStoreOp;

    VkImageLayout attachmentLayout;
};

struct BufferInfo
{
    VkBufferUsageFlagBits buffer_usage;
    VkMemoryPropertyFlags memory_properties;
    uint32_t              size;
};
