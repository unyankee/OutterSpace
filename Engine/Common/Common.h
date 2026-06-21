#pragma once

#include <volk.h>
#include <glm/glm.hpp>
#include <assert.h>

#include "glm/fwd.hpp"

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

#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define TOY_FORCEINLINE __attribute__((always_inline)) inline
#else
#define TOY_FORCEINLINE inline
#endif

FORCEINLINE uint32_t divideAndRoundUp(uint32_t numerator, uint32_t denominator)
{
	return (numerator + denominator - 1) / denominator;
}

// This one needs to map to Engine\Shaders\common.glsl layout(push_constant) uniform Constants
struct DefaultPipelineLayout
{
	VkDeviceAddress VertexDataPtr;
	VkDeviceAddress CameraDataPtr;
	VkDeviceAddress MeshletDataPtr;
	VkDeviceAddress MeshletVertexDataPtr;
	VkDeviceAddress MeshletTriangleDataPtr;
	VkDeviceAddress TransformDataPtr;
	uint32_t textureIndex;
	uint32_t samplerIndex;
	uint32_t meshletCount;
	uint32_t TransformIndex;
};

struct EditorPipelineLayout
{
	float scale[2];
	float translate[2];
	VkDeviceAddress VertexDataPtr;
};

struct Transform
{
	// will clear this part, since we should only have modelMatrix
	glm::vec4 m_position = {0, 0, 0, 0};
	glm::vec4 m_rotation = {0, 0, 0, 0};
	glm::vec4 m_scale = {1, 1, 1, 0};
	glm::mat4x4 modelMatrix = glm::mat4(1);
};

struct TransformIndex {
	uint32_t index;
};