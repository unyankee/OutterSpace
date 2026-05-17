#include <assert.h>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string>

#include <Volk/volk.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define STB_IMAGE_IMPLEMENTATION
#include <extern/stb/stb_image.h>

#include <extern/meshoptimizer/extern/fast_obj.h>

#include "meshoptimizer.h"
#include "Common/Common.h"
#include "src/PipelineManager.h"
#include "src/Camera.h"


const uint32_t StartupWidthResolution = 1920;
const uint32_t StartupHeightResolution = 1080;

using namespace ToyEngine;

// Testing vertex format
// needs to be reduced in size as well
struct Vertex
{
	float vx, vy, vz;
	float nx, ny, nz;
	float tu, tv;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct GpuCameraData {
	Mat4 view;
	Mat4 proj;
};

struct Swapchain
{
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageviews;

	uint32_t width;
	uint32_t height;
};


bool loadMesh(Mesh& outMesh, const char* path)
{
    if (!path) return false;

    std::string fullPath = std::string(ENGINE_PROJECT_ROOT) + "/" + path;
    fastObjMesh* mesh = fast_obj_read(fullPath.c_str());
    if (!mesh)
    {
       printf("Error: Could not find mesh at %s\n", fullPath.c_str());
       return false;
    }

    outMesh.vertices.clear();
    outMesh.indices.clear();

    std::vector<Vertex> unrolledVertices;
    unrolledVertices.reserve(mesh->face_count * 3); // Pre-allocate memory estimate

    unsigned int globalIndexCursor = 0;

    for (unsigned int faceIdx = 0; faceIdx < mesh->face_count; ++faceIdx)
    {
        unsigned int fv = mesh->face_vertices[faceIdx];

        std::vector<Vertex> faceVertices;
        faceVertices.reserve(fv);

        for (unsigned int vIdx = 0; vIdx < fv; ++vIdx)
        {
            fastObjIndex mi = mesh->indices[globalIndexCursor + vIdx];

            Vertex v{};
            if (mi.p)
            {
                v.vx = mesh->positions[3 * mi.p + 0];
                v.vy = mesh->positions[3 * mi.p + 1];
                v.vz = mesh->positions[3 * mi.p + 2];
            }
            if (mi.t)
            {
                v.tu = mesh->texcoords[2 * mi.t + 0];
                v.tv = mesh->texcoords[2 * mi.t + 1];
            }
            if (mi.n)
            {
                v.nx = mesh->normals[3 * mi.n + 0];
                v.ny = mesh->normals[3 * mi.n + 1];
                v.nz = mesh->normals[3 * mi.n + 2];
            }

            faceVertices.push_back(v);
        }

        // Triangulate the face 
        for (unsigned int kk = 1; kk < fv - 1; kk++)
        {
            unrolledVertices.push_back(faceVertices[0]);
            unrolledVertices.push_back(faceVertices[kk + 1]);
            unrolledVertices.push_back(faceVertices[kk]);
        }

        globalIndexCursor += fv;
    }
	
    size_t totalIndices = unrolledVertices.size();
    std::vector<unsigned int> remap(totalIndices);
	
    size_t uniqueVertexCount = meshopt_generateVertexRemap(
        remap.data(), 
        nullptr, 
        totalIndices, 
        unrolledVertices.data(), 
        totalIndices, 
        sizeof(Vertex)
    );

    
    outMesh.vertices.resize(uniqueVertexCount);
    outMesh.indices.resize(totalIndices);

    
    meshopt_remapIndexBuffer(outMesh.indices.data(), nullptr, totalIndices, remap.data());
    meshopt_remapVertexBuffer(outMesh.vertices.data(), unrolledVertices.data(), totalIndices, sizeof(Vertex), remap.data());

    meshopt_optimizeVertexCache(outMesh.indices.data(), outMesh.indices.data(), totalIndices, uniqueVertexCount);

    fast_obj_destroy(mesh);
    return true;
}

// Temporary class to hold all the relevant engine data
// is not API agnostic (just yet)
// need to investigate other API's as well to
// determine the best layout and not building a
// vulkan only API
// Also, this class will be refactored as the project
// progresses
class EngineInstance
{
public:
	// Variables // 
	VkInstance Instance;
	// Collection of available physical devices
	std::vector<VkPhysicalDevice> PhysicalDevices;
	// The device instance selected to create the device 
	// Restricted right now for 1 PhysicalDevice and 1 device
	// This is a auto-imposed restriction for now since you can have multiple
	// devices out of the same physical device, but keeping it simple for now
	VkPhysicalDevice PhysicalDevice;
	//
	GLFWwindow* window;
	//
	VkDevice Device;
	//	
	VkSurfaceKHR surface;
	// 
	VkSurfaceCapabilitiesKHR SurfaceCaps;
	//	
	VkSurfaceFormatKHR surfaceFormat;
	//
	//VkSwapchainKHR Swapchain;
	//
	uint32_t FamilyIndex = 0;
	//
	uint32_t swapchainImagesCount = 0;
	//
	//std::vector<VkImage> SwapchainImages;
	//
	//VkRenderPass RenderPass;

	//VkPipeline MeshPipeline;
	//VkPipelineLayout MeshPipelineLayout;

	Swapchain swapchain;
	RenderTarget DepthTexture;
	
	VkPhysicalDeviceMemoryProperties PhysicalMemoryProperties;

	VkDebugReportCallbackEXT DebugCallback;
	// Functions //
	// This is not the best naming
	// Since Engine::InitInstance is way more general than what
	// this function is actually doing.
	// Should consider move it / rename it once what 
	// it does changes over time
	//

	PipelineManager pipeline_manager;
	//
	Camera camera;

	Buffer camera_buffer;
	//

	//
	void MainLoop();
	//
	void InitInstance();
	// Iterate over the list of physical devices and pick the one that
	// looks better or matches better the requirements of the engine
	void SelectPhysicalDevice();
	// Create a single device instance out of the physical device 
	void CreateDevice();
	// Create/Get surface for rendering
	void CreateSurface();
	// 
	void GetSwapchainFormat();
	//
	TextureResource LoadTexture(const char* filename);

	// tmp helpers to aid creating / copying / transitions	
	VkCommandBuffer beginImmediateCommandBuffer(VkCommandPool tempPool);
	void endImmediateCommandBuffer(VkCommandBuffer cmdBuffer, VkCommandPool tempPool);
	void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);
	void copyBufferToImage(VkCommandBuffer cmd, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);
	
	void CreateSwapchain();
	void CreateDepthTexture();
	//
	//void createRenderpass();
	//
	void createPersistentlyMappedBuffer(Buffer& outBuffer, uint32_t size, VkBufferUsageFlags usageFlags);
	void createBuffer(Buffer& outBuffer, uint32_t size, VkBufferUsageFlags usageFlags, const void* data = nullptr, bool bPersistentlyMapped = false);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool);
	void DestroyBuffer(Buffer& Buffer);
	uint32_t selectMemoryType(const uint32_t memoryTypeBits, VkMemoryPropertyFlags flags);
	
	


	uint32_t getGraphicsQueueFamily();

	void RegisterDebugCallback();

	static VkImageMemoryBarrier ImageBarrier(VkImage Image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkShaderModule loadShader(const char* shaderPath) const;

	VkCommandPool CreateCommandPool();
};


VkCommandBuffer EngineInstance::beginImmediateCommandBuffer(VkCommandPool tempPool)
{
	VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = tempPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	VK_CHECK(vkAllocateCommandBuffers(Device, &allocInfo, &cmdBuffer));

	VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	return cmdBuffer;
}

void EngineInstance::endImmediateCommandBuffer(VkCommandBuffer cmdBuffer, VkCommandPool tempPool)
{
	VK_CHECK(vkEndCommandBuffer(cmdBuffer));

	VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	VkQueue queue;
	vkGetDeviceQueue(Device, FamilyIndex, 0, &queue);

	VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(queue));

	vkFreeCommandBuffers(Device, tempPool, 1, &cmdBuffer);
}

void EngineInstance::transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout,
                                           VkImageLayout newLayout, VkPipelineStageFlags srcStage,
                                           VkPipelineStageFlags dstStage)
{
	VkImageMemoryBarrier barrier = ImageBarrier(image, 0, 0, oldLayout, newLayout);

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}
	vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void EngineInstance::copyBufferToImage(VkCommandBuffer cmd, VkBuffer srcBuffer, VkImage dstImage, uint32_t width,
                                       uint32_t height)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(cmd, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

TextureResource EngineInstance::LoadTexture(const char* filename)
{
	TextureResource outTexture{};

	std::string fullPath = std::string(ENGINE_PROJECT_ROOT) + "/" + filename;
	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels = stbi_load(fullPath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels)
	{
		printf("Failed to load texture image: %s\n", fullPath.c_str());
		return outTexture;
	}

	uint32_t imageSize = static_cast<uint32_t>(texWidth * texHeight * 4);

	Buffer stagingBuffer{};
	createBuffer(stagingBuffer, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, pixels, true);
	stbi_image_free(pixels);

	VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkCreateImage(Device, &imageInfo, nullptr, &outTexture.image);

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(Device, outTexture.image, &memReqs);

	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize = memReqs.size;
	allocInfo.memoryTypeIndex = selectMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(Device, &allocInfo, nullptr, &outTexture.memory);
	vkBindImageMemory(Device, outTexture.image, outTexture.memory, 0);

	VkCommandPool tempPool = CreateCommandPool();
	VkCommandBuffer cmd = beginImmediateCommandBuffer(tempPool);

	transitionImageLayout(cmd, outTexture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	copyBufferToImage(cmd, stagingBuffer.buffer, outTexture.image, imageInfo.extent.width, imageInfo.extent.height);

	transitionImageLayout(cmd, outTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	                      VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	endImmediateCommandBuffer(cmd, tempPool);

	DestroyBuffer(stagingBuffer);

	VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewInfo.image = outTexture.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.layerCount = 1;
	vkCreateImageView(Device, &viewInfo, nullptr, &outTexture.view);

	//outTexture.bindlessIndex = globalBindlessManager.RegisterTexture(outTexture.view);
	outTexture.bindlessIndex = 0;

	return outTexture;
}


void EngineInstance::MainLoop()
{
	VkSemaphore AquireSemaphone;
	VkSemaphore submitSemaphore;
	VkSemaphoreCreateInfo SemaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &AquireSemaphone);
	vkCreateSemaphore(Device, &SemaphoreCreateInfo, nullptr, &submitSemaphore);

	VkQueue Queue;
	vkGetDeviceQueue(Device, FamilyIndex, 0, &Queue);

	VkCommandPool CommandPool = CreateCommandPool();

	VkCommandBufferAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	AllocateInfo.commandPool = CommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = 1;

	VkCommandBuffer CommandBuffer;
	VK_CHECK(vkAllocateCommandBuffers(Device, &AllocateInfo, &CommandBuffer));

	VkShaderModule MeshVs = loadShader("Shaders/mesh.vert.spv");
	VkShaderModule MeshFs = loadShader("Shaders/mesh.frag.spv");
	
	pipeline_manager.createMeshPipeline(MeshVs, MeshFs, surfaceFormat.format);
	//createMeshPipeline(MeshVs, MeshFs);

	Mesh testMesh;
	if (!loadMesh(testMesh, "assets/models/untitled.obj"))
	{
		// assert for now, hard crash is better to find this
		assert(!"");
	}

	TextureResource texture = LoadTexture("assets/models/Dragon_Bump_Col2.jpg");
	pipeline_manager.AddTextureToGlobalDescriptorSet(texture);
	
	Buffer vb, ib;
	createBuffer(vb, static_cast<uint32_t>(testMesh.vertices.size() * sizeof(Vertex)), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, testMesh.vertices.data());
	createBuffer(ib, static_cast<uint32_t>(testMesh.indices.size() * sizeof(uint32_t)), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, testMesh.indices.data());

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		{
			// Resizing
			int32_t Width = 0;
			int32_t Heigh = 0;
			glfwGetWindowSize(window, &Width, &Heigh);
			if (swapchain.width != Width || swapchain.height != Heigh)
			{
				CreateSwapchain();
				camera.setPerspective(70.f, (float)Width / (float)Heigh, 0.1f, 1000.f);
			}
		}

		// Update camera data on GPU
		camera.update();
		GpuCameraData camData;
		camData.view = camera.getViewMatrix();
		camData.proj = camera.getProjectionMatrix();
		memcpy(camera_buffer.data, &camData, sizeof(GpuCameraData));

		// Main rendering loop for now
		uint32_t ImageIndex = 0;
		VK_CHECK(
			vkAcquireNextImageKHR(Device, swapchain.swapchain, ~0ull, AquireSemaphone, VK_NULL_HANDLE, &ImageIndex));

		VK_CHECK(vkResetCommandPool(Device, CommandPool, 0))


		VkCommandBufferBeginInfo BeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;


		VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

		const VkImageMemoryBarrier ColorBarrier = ImageBarrier(
			swapchain.images[ImageIndex],
			0,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
		const VkImageMemoryBarrier DepthBarrier = ImageBarrier(
			DepthTexture.image,
			0,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);

		VkImageMemoryBarrier Barriers[] = {ColorBarrier, DepthBarrier};

		vkCmdPipelineBarrier(
			CommandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			0, nullptr,
			2, Barriers
		);

		constexpr VkClearColorValue ClearColorValue = {27.0f / 255.0f, 3.0f / 255.0f, 3.0f / 255.0f, 1.0f};
		VkClearValue ClearValue = {ClearColorValue};

		VkClearValue depthClear;
		depthClear.depthStencil = { 1.0f, 0 };
		
		VkRenderingAttachmentInfo ColorAttachment = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
		ColorAttachment.imageView = swapchain.imageviews[ImageIndex];
        ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.clearValue = ClearValue;

		VkRenderingAttachmentInfo DepthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        DepthAttachment.imageView = DepthTexture.imageView;
        DepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        DepthAttachment.clearValue = depthClear;

        VkRenderingInfo RenderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        RenderingInfo.renderArea.extent = { swapchain.width, swapchain.height };
        RenderingInfo.layerCount = 1;
        RenderingInfo.colorAttachmentCount = 1;
        RenderingInfo.pColorAttachments = &ColorAttachment;
		RenderingInfo.pDepthAttachment = &DepthAttachment;

        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);

		VkViewport Viewport = { 0, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
		vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

		VkRect2D Scissor = { 0, 0, (swapchain.width), (swapchain.height) };
		vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_manager.TmpPipeline.Pipeline);

		vkCmdBindDescriptorSets(
			CommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline_manager.TmpPipeline.PipelineLayout,
			0, 1, &pipeline_manager.globalBindlessDescriptorSet,
			0, nullptr
		);

		const uint32_t dummyTextureIndex = 0;
		const uint32_t dummySamplerIndex = 0;

		DefaultPipelineLayout PushData
		{
			vb.gpuAddress, camera_buffer.gpuAddress, dummyTextureIndex, dummySamplerIndex 
		};
		
		vkCmdPushConstants(CommandBuffer, pipeline_manager.TmpPipeline.PipelineLayout,VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DefaultPipelineLayout), &PushData);

		vkCmdBindIndexBuffer(CommandBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(CommandBuffer, static_cast<uint32_t>(testMesh.indices.size()), 1, 0, 0, 0);

		vkCmdEndRendering(CommandBuffer);

		const VkImageMemoryBarrier RenderEndBarrier = ImageBarrier(
			swapchain.images[ImageIndex], 
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
			0, 
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		);

		vkCmdPipelineBarrier(
			CommandBuffer, 
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          
			VK_DEPENDENCY_BY_REGION_BIT, 
			0, nullptr, 
			0, nullptr, 
			1, &RenderEndBarrier
		);
		
		//vkCmdClearColorImage(CommandBuffer, SwapchainImages[ImageIndex], VK_IMAGE_LAYOUT_GENERAL, &ClearColorValue, 1, &ImageSubresourceRamge);

		VK_CHECK(vkEndCommandBuffer(CommandBuffer));

		VkPipelineStageFlags PipelineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.pWaitDstStageMask = &PipelineStageFlags;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &AquireSemaphone;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CommandBuffer;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &submitSemaphore;

		vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);

		VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		PresentInfo.pImageIndices = &ImageIndex;
		PresentInfo.pWaitSemaphores = &submitSemaphore;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pSwapchains = &swapchain.swapchain;
		PresentInfo.swapchainCount = 1;

		VK_CHECK(vkQueuePresentKHR(Queue, &PresentInfo));
		
		// Performance tracking
		static double lastTime = glfwGetTime();
		static int nbFrames = 0;
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 0.01) {
			char title[256];
			sprintf(title, "OutterSpace - FPS: %.1f (%.2f ms)", double(nbFrames), 1000.0 / double(nbFrames));
			glfwSetWindowTitle(window, title);
			nbFrames = 0;
			lastTime += 1.0;
		}

		// This is bad. here just for purely testing 
		VK_CHECK(vkDeviceWaitIdle(Device))
	}

	VK_CHECK(vkDeviceWaitIdle(Device));
	// Expected releasing resources

}

VkCommandPool EngineInstance::CreateCommandPool()
{
	VkCommandPoolCreateInfo CommandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	CommandPoolCreateInfo.queueFamilyIndex = FamilyIndex;
	CommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	VkCommandPool CommandPool = VK_NULL_HANDLE;
	VK_CHECK(vkCreateCommandPool(Device, &CommandPoolCreateInfo, nullptr, &CommandPool));
	return CommandPool;
}

void EngineInstance::InitInstance()
{
	int InitResult = glfwInit();
	assert(InitResult);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	glfwInitHint(GLFW_CLIENT_API, GLFW_NO_API);

	volkInitialize();

	VkApplicationInfo ApplicationInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	ApplicationInfo.apiVersion = VK_API_VERSION_1_4;

	VkInstanceCreateInfo InstanceInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	InstanceInfo.pApplicationInfo = &ApplicationInfo;

	const char* Extensions[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
	};

	InstanceInfo.ppEnabledExtensionNames = Extensions;
	InstanceInfo.enabledExtensionCount = ARRAY_SIZE(Extensions);

#ifdef _DEBUG
	const char* DebugLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	InstanceInfo.ppEnabledLayerNames = DebugLayers;
	InstanceInfo.enabledLayerCount = ARRAY_SIZE(DebugLayers);

#endif


	VK_CHECK(vkCreateInstance(&InstanceInfo, nullptr, &Instance));
	//
	volkLoadInstance(Instance);

	RegisterDebugCallback();

	uint32_t DeviceCount = 0;
	assert(PhysicalDevices.empty());
	VK_CHECK(vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr));
	PhysicalDevices.resize(DeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(Instance, &DeviceCount, PhysicalDevices.data()));
	// with this data we can start selecting which device is the best one for us

	const uint32_t WindowWidth = StartupWidthResolution;
	const uint32_t WindowHeigh = StartupHeightResolution;
	window = glfwCreateWindow(WindowWidth, WindowHeigh, "OutterSpace", 0, 0);

	SelectPhysicalDevice();
	getGraphicsQueueFamily();
	CreateDevice();
	//
	volkLoadDevice(Device);
	pipeline_manager.init(Device);

	camera.setPerspective(70.f, (float)StartupWidthResolution / (float)StartupHeightResolution, 0.1f, 1000.f);
	camera.update();

	createBuffer(camera_buffer, sizeof(GpuCameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, nullptr, true);

	CreateSurface();
	GetSwapchainFormat();
	//GetOrCreateSwapchainImages();
	CreateSwapchain();
	
	//createImageView();
	//createFramebuffer();
}

void EngineInstance::SelectPhysicalDevice()
{
	assert(!PhysicalDevices.empty());
	printf("Checking physical devices, detected %llu\n", PhysicalDevices.size());

	uint32_t DeviceCandidate = 0xDEAD;
	for (uint32_t i = 0; i < PhysicalDevices.size(); i++)
	{
		VkPhysicalDeviceProperties Properties;
		vkGetPhysicalDeviceProperties(PhysicalDevices[i], &Properties);

		printf("Physical Device Name: %s\n", Properties.deviceName);
		printf("Physical Device Type: %d\n", Properties.deviceType);

		// We want to pick first a dedicated gpu
		if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			DeviceCandidate = i;
		}
	}

	if (DeviceCandidate == 0xDEAD && !PhysicalDevices.empty())
	{
		printf("There is no candidate dedicated physical device, falling back to first gpu found\n");
		DeviceCandidate = 0;
		PhysicalDevice = PhysicalDevices[0];
	}
	else
	{
		PhysicalDevice = PhysicalDevices[DeviceCandidate];
	}
	VkPhysicalDeviceProperties Properties;
	vkGetPhysicalDeviceProperties(PhysicalDevices[DeviceCandidate], &Properties);
	printf("Selected Physical Device Name: %s\n", Properties.deviceName);
	
	// TODO: Maybe add here all the relevant calls to physical device on init time? 
	vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalMemoryProperties);
}

void EngineInstance::CreateDevice()
{
	assert(PhysicalDevice != VK_NULL_HANDLE);

	// This is not correct,
	// Validation layers will complain about it
	// But at the moment this is a shortcut
	constexpr float QueuePriorities[] = { 1.0f };
	VkDeviceQueueCreateInfo DeviceQueueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	DeviceQueueCreateInfo.queueFamilyIndex = FamilyIndex;
	DeviceQueueCreateInfo.queueCount = 1;
	DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;


	
	VkDeviceCreateInfo DeviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	DeviceCreateInfo.queueCreateInfoCount = 1;
	DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;

	const char* Extensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)

#endif
	};

	DeviceCreateInfo.ppEnabledExtensionNames = Extensions;
	DeviceCreateInfo.enabledExtensionCount = ARRAY_SIZE(Extensions);

	VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.runtimeDescriptorArray = VK_TRUE;               
	features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE; 
	features12.descriptorBindingPartiallyBound = VK_TRUE;
	features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    
	features12.bufferDeviceAddress = VK_TRUE; 
	features12.pNext = nullptr; 

	VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
	DynamicRenderingFeatures.dynamicRendering = VK_TRUE;
	DynamicRenderingFeatures.pNext = &features12;

	VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	features2.features.shaderInt64 = VK_TRUE; 
	features2.pNext = &DynamicRenderingFeatures; 

	DeviceCreateInfo.pNext = &features2;

	VK_CHECK(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device));
}

void EngineInstance::CreateSurface()
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	VkWin32SurfaceCreateInfoKHR WindowCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	WindowCreateInfo.hinstance = GetModuleHandle(0);
	WindowCreateInfo.hwnd = glfwGetWin32Window(window);

	vkCreateWin32SurfaceKHR(Instance, &WindowCreateInfo, nullptr, &surface);
#else
	assert(false);
#endif
}

void EngineInstance::GetSwapchainFormat()
{
	uint32_t DeviceSurfaceCount = 0;
	std::vector<VkSurfaceFormatKHR> SupportedFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &DeviceSurfaceCount, nullptr);
	SupportedFormats.resize(DeviceSurfaceCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &DeviceSurfaceCount, SupportedFormats.data());

	uint32_t Candidate = 0;
	for (uint32_t i = 0; i < DeviceSurfaceCount; i++)
	{
		if (SupportedFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
		{
			Candidate = i;
		}
	}

	surfaceFormat = SupportedFormats[Candidate];
}

void EngineInstance::CreateDepthTexture()
{
	if (DepthTexture.imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(Device, DepthTexture.imageView, nullptr);
	}
	if (DepthTexture.image != VK_NULL_HANDLE)
	{
		vkDestroyImage(Device, DepthTexture.image, nullptr);
	}
	if (DepthTexture.memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(Device, DepthTexture.memory, nullptr);
	}

	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; 

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapchain.width;
    imageInfo.extent.height = swapchain.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(Device, &imageInfo, nullptr, &DepthTexture.image));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(Device, DepthTexture.image, &memReqs);

    uint32_t memoryTypeIndex = selectMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VK_CHECK(vkAllocateMemory(Device, &allocInfo, nullptr, &DepthTexture.memory));
    VK_CHECK(vkBindImageMemory(Device, DepthTexture.image, DepthTexture.memory, 0));

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = DepthTexture.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; 
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(Device, &viewInfo, nullptr, &DepthTexture.imageView));
}

void EngineInstance::CreateSwapchain()
{
	// TODO: Needed for recreating it? 
	vkDeviceWaitIdle(Device);

	if (swapchain.swapchain != VK_NULL_HANDLE)
	{
		for (auto imageView : swapchain.imageviews)
		{
			vkDestroyImageView(Device, imageView, nullptr);
		}
		// Do NOT destroy the old swapchain yet; we pass it to SwapchainCreateInfo.oldSwapchain
	}
	
	VkSwapchainKHR LocalSwapchain = VK_NULL_HANDLE;

	VkCompositeAlphaFlagBitsKHR CompositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	{
		//Query device caps
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysicalDevice, surface, &SurfaceCaps));
		CompositeAlphaFlags = (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) ?
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
			: (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) ?
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
			: (SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) ?
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}

	int32_t Width = 0;
	int32_t Heigh = 0;
	glfwGetFramebufferSize(window, &Width, &Heigh);

	VkSwapchainCreateInfoKHR SwapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	SwapchainCreateInfo.surface = surface;
	
	uint32_t imageCount = SurfaceCaps.minImageCount + 1;
	if (SurfaceCaps.maxImageCount > 0 && imageCount > SurfaceCaps.maxImageCount)
	{
		imageCount = SurfaceCaps.maxImageCount;
	}
	SwapchainCreateInfo.minImageCount = imageCount;

	SwapchainCreateInfo.imageFormat = surfaceFormat.format;
	SwapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	SwapchainCreateInfo.imageExtent.width = Width;
	SwapchainCreateInfo.imageExtent.height = Heigh;
	SwapchainCreateInfo.imageArrayLayers = 1;
	SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	SwapchainCreateInfo.queueFamilyIndexCount = 1;
	SwapchainCreateInfo.pQueueFamilyIndices = &FamilyIndex;
	SwapchainCreateInfo.compositeAlpha = CompositeAlphaFlags;
	SwapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	// Query supported present modes
	uint32_t presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, surface, &presentModeCount, nullptr));
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(PhysicalDevice, surface, &presentModeCount, presentModes.data()));

	// Default to FIFO
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// Try to find the one we are interested
	for (const auto& mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			swapchainPresentMode = mode;
			break;
		}
	}

	SwapchainCreateInfo.presentMode = swapchainPresentMode;
	SwapchainCreateInfo.oldSwapchain = swapchain.swapchain;

	VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, nullptr, &LocalSwapchain));

	if (swapchain.swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(Device, swapchain.swapchain, nullptr);
	}
	// Remove the old one when used, assuming we safely got the new one
	
	VK_CHECK(vkDeviceWaitIdle(Device));

	std::vector<VkImage> images;
	std::vector<VkImageView> imageviews;

	swapchainImagesCount = 0;
	vkGetSwapchainImagesKHR(Device, LocalSwapchain, &swapchainImagesCount, nullptr);
	images.resize(swapchainImagesCount);
	vkGetSwapchainImagesKHR(Device, LocalSwapchain, &swapchainImagesCount, images.data());

	imageviews.resize(swapchainImagesCount);
	for (uint32_t i = 0; i < swapchainImagesCount; ++i)
	{
		VkImageViewCreateInfo ImageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ImageViewCreateInfo.image = images[i];
		ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ImageViewCreateInfo.format = surfaceFormat.format;

		ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageViewCreateInfo.subresourceRange.levelCount = 1;
		ImageViewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &imageviews[i]))
	}

	swapchain.swapchain = LocalSwapchain;
	swapchain.imageviews = imageviews;
	swapchain.images = images;

	swapchain.width = Width;
	swapchain.height = Heigh;

	// Right now, since this is the same as the swapchain for testing, need to keep them paired,
	// will move fast to blitting the result to the swapchain so I can detach them
	CreateDepthTexture();
	
	VK_CHECK(vkDeviceWaitIdle(Device));
}

void EngineInstance::DestroyBuffer(Buffer& Buffer)
{
	if (Buffer.data)
	{
		vkFreeMemory(Device, Buffer.memory, nullptr);
		vkDestroyBuffer(Device, Buffer.buffer, nullptr);
	}
}

void EngineInstance::createPersistentlyMappedBuffer(Buffer& outBuffer, uint32_t size, VkBufferUsageFlags usageFlags)
{
	VkBufferCreateInfo Createinfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	Createinfo.size = size;
	Createinfo.usage = usageFlags;

	VkBuffer buffer = VK_NULL_HANDLE;
	VK_CHECK(vkCreateBuffer(Device, &Createinfo, nullptr, &buffer));
	// Memory reqs
	VkMemoryRequirements memoryReqs;
	vkGetBufferMemoryRequirements(Device, buffer, &memoryReqs);

	uint32_t memoryTypeIndex = selectMemoryType(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkMemoryAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	AllocateInfo.memoryTypeIndex = memoryTypeIndex;
	AllocateInfo.allocationSize = memoryReqs.size;

	VkMemoryAllocateFlagsInfo allocFlagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
	allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
	
	// If the usage includes BDA, store the address in the outBuffer struct
	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		// Bindless support
		AllocateInfo.pNext = &allocFlagsInfo;
	}

	VkDeviceMemory memory = VK_NULL_HANDLE;
	VK_CHECK(vkAllocateMemory(Device, &AllocateInfo, nullptr, &memory));

	VK_CHECK(vkBindBufferMemory(Device, buffer, memory, 0));
	
	void* Data;
	VK_CHECK(vkMapMemory(Device, memory, 0, VK_WHOLE_SIZE, 0, &Data));

	outBuffer.buffer = buffer;	
	outBuffer.data = Data;	
	outBuffer.memory = memory;	
	outBuffer.size = size;	
	outBuffer.gpuAddress = 0;

	if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		VkBufferDeviceAddressInfo addressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
		addressInfo.buffer = buffer;
		outBuffer.gpuAddress = vkGetBufferDeviceAddress(Device, &addressInfo);
	}
}

void EngineInstance::createBuffer(Buffer& outBuffer, uint32_t size, VkBufferUsageFlags usageFlags, const void* data, bool bPersistentlyMapped)
{
	if (bPersistentlyMapped)
	{
		createPersistentlyMappedBuffer(outBuffer, size, usageFlags);
		if (data)
		{
			memcpy(outBuffer.data, data, size);
		}
		else
		{
			// error reporting, no data passed to this buffer?
		}
	}
	else
	{
		VkBufferCreateInfo Createinfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		Createinfo.size = size;
		Createinfo.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkBuffer buffer = VK_NULL_HANDLE;
		VK_CHECK(vkCreateBuffer(Device, &Createinfo, nullptr, &buffer));

		VkMemoryRequirements memoryReqs;
		vkGetBufferMemoryRequirements(Device, buffer, &memoryReqs);

		uint32_t memoryTypeIndex = selectMemoryType(memoryReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkMemoryAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		AllocateInfo.memoryTypeIndex = memoryTypeIndex;
		AllocateInfo.allocationSize = memoryReqs.size;

		VkMemoryAllocateFlagsInfo allocFlagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
		allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		
		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			AllocateInfo.pNext = &allocFlagsInfo;
		}

		VkDeviceMemory memory = VK_NULL_HANDLE;
		VK_CHECK(vkAllocateMemory(Device, &AllocateInfo, nullptr, &memory));
		VK_CHECK(vkBindBufferMemory(Device, buffer, memory, 0));

		outBuffer.buffer = buffer;
		outBuffer.data = nullptr;
		outBuffer.memory = memory;
		outBuffer.size = size;
		outBuffer.gpuAddress = 0;

		if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			VkBufferDeviceAddressInfo addressInfo = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
			addressInfo.buffer = buffer;
			outBuffer.gpuAddress = vkGetBufferDeviceAddress(Device, &addressInfo);
		}

		if (data)
		{
			Buffer stagingBuffer;
			createPersistentlyMappedBuffer(stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
			memcpy(stagingBuffer.data, data, size);

			VkCommandPool tempPool = CreateCommandPool();
			copyBuffer(stagingBuffer.buffer, outBuffer.buffer, size, tempPool);
			DestroyBuffer(stagingBuffer);
		}
	}
}


void EngineInstance::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkCommandPool commandPool)
{
	VkCommandBuffer cmd = beginImmediateCommandBuffer(commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

	endImmediateCommandBuffer(cmd, commandPool);
}


uint32_t EngineInstance::selectMemoryType(const uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
{
	for (uint32_t i = 0; i < PhysicalMemoryProperties.memoryTypeCount; ++i) 
	{
		if ((memoryTypeBits & (1 << i)) != 0 && (PhysicalMemoryProperties.memoryTypes[i].propertyFlags & flags) == flags) 
		{
			return  i;
		}
	}
	
	assert(!"");

	return -1;
}



uint32_t EngineInstance::getGraphicsQueueFamily()
{
	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueCount, nullptr);
	
	std::vector<VkQueueFamilyProperties> queues(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &queueCount, queues.data());

	for (uint32_t i = 0; i < queueCount; ++i)
	{
		if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			FamilyIndex = i;
			return 1;
		}
	}

	assert(!"No queue families support graphics");
	return -1;
}

VkBool32 DebugReporterVulkan(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
	size_t location,
	int32_t messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* pUserData)
{
	const char* Type = (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ?
		"ERROR"
		: (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		? "WARNING"
		: "INFO";

	// Most simple implementation of error / warning reporting
	// want an error free application
	printf("%s : %s \n", Type, pMessage);

	return VK_FALSE;
};


void EngineInstance::RegisterDebugCallback()
{
	VkDebugReportCallbackCreateInfoEXT CreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
	CreateInfo.pfnCallback = &DebugReporterVulkan;
	CreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;

	VkDebugReportCallbackEXT DebugAllocatorCallback = { VK_NULL_HANDLE };
	VK_CHECK(vkCreateDebugReportCallbackEXT(Instance, &CreateInfo, 0, &DebugAllocatorCallback));

	DebugCallback = DebugAllocatorCallback;
}

VkImageMemoryBarrier EngineInstance::ImageBarrier(VkImage Image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
	VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier Result = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	Result.srcAccessMask = srcAccessMask;
	Result.dstAccessMask = dstAccessMask;
	Result.oldLayout = oldLayout;
	Result.newLayout = newLayout;
	Result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	Result.image = Image;
	Result.subresourceRange.aspectMask = (dstAccessMask == VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	Result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	Result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return Result;
}

VkShaderModule EngineInstance::loadShader(const char* shaderPath) const
{
	std::string fullPath = std::string(ENGINE_DIR) + "/" + shaderPath;
	FILE* file = fopen(fullPath.c_str(), "rb");
	if (!file)
	{
		printf("Error: Could not find shader at %s\n", fullPath.c_str());
		assert(file);
	}

	fseek(file, 0, SEEK_END);
	uint32_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* code = new char[size];
	fread(code, size, 1, file);
	fclose(file);

	VkShaderModuleCreateInfo ShaderModuleCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	ShaderModuleCreateInfo.codeSize = size;
	ShaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code);

	VkShaderModule ShaderModule;
	VK_CHECK(vkCreateShaderModule(Device, &ShaderModuleCreateInfo, nullptr, &ShaderModule));

	delete[] code;
	return ShaderModule;
}


int main()
{
	EngineInstance Engine;
	Engine.InitInstance();

	Engine.MainLoop();

	glfwDestroyWindow(Engine.window);


	return 0;
}
