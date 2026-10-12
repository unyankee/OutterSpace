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

// This is incorrectly used as well, GPU should not use this one, there should be a lightweight version of just 4x3 mat
// cutting corners for now
struct Transform
{
	// will clear this part, since we should only have modelMatrix
	glm::vec4 m_position = {0, 0, 0, 0};
	glm::vec4 m_rotation = {0, 0, 0, 0};
	glm::vec4 m_scale = {1, 1, 1, 0};
	glm::mat4x4 modelMatrix = glm::mat4(1);
};

struct GPUTransform
{
	glm::mat4x3 modelMatrix = glm::mat4(1);
};


constexpr uint32_t MaxSupportedTexturess = 8;
struct TextureCollection
{
	uint32_t Textures[MaxSupportedTexturess];
	uint32_t TextureCount = 0;
	uint32_t _padding[3];
};

// when rendering could iterate over Mesh+Material combo, and query transform idx?
struct Material
{
	// Should this hold the textures?
	// can have some settings to auto 
	// set in the pipeline.
	// stuff like backface culling, two-sided, 
	// depth bias, etc...
	TextureCollection textureCollection;
};