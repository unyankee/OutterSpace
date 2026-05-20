#pragma once

#include <volk.h>
#include <cstdint>
#include <vector>

namespace ToyEngine
{

    struct GpuContext
    {
        VkDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceMemoryProperties m_memoryProperties{};
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        uint32_t m_graphicsFamilyIndex = 0;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    };

    class ResourceManager;

    struct Buffer
    {
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDeviceAddress m_gpuAddress = 0;
        void* m_data = nullptr;
        uint32_t m_size = 0;

        void* map(const GpuContext& ctx);
        void unmap(const GpuContext& ctx);
        void copyDataToBuffer(const void* data, uint32_t size) const;

    private:
        friend class ResourceManager;
        friend struct Texture;
        void create(const GpuContext& ctx, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void* initialData = nullptr);
        void destroy(const GpuContext& ctx);
    };

    struct Texture
    {
        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        uint32_t m_width = 0;
        uint32_t m_height = 0;
        uint32_t m_bindlessIndex = 0;

        void uploadData(const GpuContext& ctx, const void* data, uint32_t size);

    private:
        friend class ResourceManager;
        void load(const GpuContext& ctx, const char* path);
        void create(const GpuContext& ctx, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);
        void destroy(const GpuContext& ctx);
    };

    struct RenderTarget
    {
        VkImage m_image = VK_NULL_HANDLE;
        VkImageView m_view = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;

        VkFormat m_format;

        uint32_t m_width = 0;
        uint32_t m_height = 0;

    private:
        friend class ResourceManager;
        void create(const GpuContext& ctx, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);
        void destroy(const GpuContext& ctx);
    };

}

