#pragma once

#include <volk.h>
#include <cstdint>
#include <vector>

namespace ToyEngine {

    struct GpuContext {
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        VkCommandPool commandPool;
        VkQueue graphicsQueue;
        uint32_t graphicsFamilyIndex;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    };

    struct Buffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceAddress gpuAddress = 0;
        void* data = nullptr;
        uint32_t size = 0;

        void create(const GpuContext& ctx, uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void* initialData = nullptr);

        void destroy(const GpuContext& ctx);

        void copyDataToBuffer(const void* data, uint32_t size);
        };

    struct Texture {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t bindlessIndex = 0;

        void load(const GpuContext& ctx, const char* path);
        void destroy(const GpuContext& ctx);
    };

    struct RenderTarget {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkFormat format;
        uint32_t width = 0;
        uint32_t height = 0;

        void create(const GpuContext& ctx, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);
        void destroy(const GpuContext& ctx);
    };

}
