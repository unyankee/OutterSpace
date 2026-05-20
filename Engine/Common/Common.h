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

struct DefaultPipelineLayout
{
	VkDeviceAddress VertexDataPtr;
	VkDeviceAddress CameraDataPtr;
	VkDeviceAddress MeshletDataPtr;
	VkDeviceAddress MeshletVertexDataPtr;
	VkDeviceAddress MeshletTriangleDataPtr;
	uint32_t textureIndex;
	uint32_t samplerIndex;
};
