#pragma once

#include <volk.h>
#include <cstdio>
#include <assert.h>

#define VK_CHECK(VK_FUNCTION) \
	do { \
		VkResult Result = VK_FUNCTION; \
		if (Result != VK_SUCCESS) { \
			printf("Vulkan Error: %d at %s:%d\n", Result, __FILE__, __LINE__); \
			fflush(stdout); \
			assert(Result == VK_SUCCESS); \
		} \
	} while (0);

#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))


struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceAddress gpuAddress;
	void* data;
	uint32_t size;
};

struct RenderTarget
{
	VkImage image = VK_NULL_HANDLE;
	VkImageView imageView = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE; 
};

struct TextureResource
{
	VkImage image = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	uint32_t bindlessIndex = 0;
};

struct DefaultPipelineLayout
{
	VkDeviceAddress VertexDataPtr;
	VkDeviceAddress CameraDataPtr;
	uint32_t textureIndex;
	uint32_t samplerIndex;
};