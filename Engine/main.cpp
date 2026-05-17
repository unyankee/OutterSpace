#include <assert.h>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string>

#include <Volk/volk.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <extern/meshoptimizer/extern/fast_obj.h>
#include "Common/Common.h"
#include "src/PipelineManager.h"


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

bool loadMesh(Mesh& outMesh, const char* path)
{
	if (!path)
	{
		return false;
	}

	std::string fullPath = std::string(ENGINE_PROJECT_ROOT) + "/" + path;
	fastObjMesh* mesh = fast_obj_read(fullPath.c_str());
	if (!mesh)
	{
		printf("Error: Could not find mesh at %s\n", fullPath.c_str());
		return false;
	}

	outMesh.vertices.resize(mesh->index_count);
	outMesh.indices.resize(mesh->index_count);

	for (unsigned int ii = 0; ii < mesh->group_count; ii++)
	{
		const fastObjGroup& grp = mesh->groups[ii];

		int idx = 0;
		for (unsigned int jj = 0; jj < grp.face_count; jj++)
		{
			unsigned int fv = mesh->face_vertices[grp.face_offset + jj];

			for (unsigned int kk = 0; kk < fv; kk++)
			{
				fastObjIndex mi = mesh->indices[grp.index_offset + idx];

				Vertex& vertexData = outMesh.vertices[idx];

				if (mi.p)
				{
					vertexData.vx = mesh->positions[3 * mi.p + 0];
					vertexData.vy = mesh->positions[3 * mi.p + 1];
					vertexData.vz = mesh->positions[3 * mi.p + 2];
				}

				if (mi.t)
				{
					vertexData.tu = mesh->texcoords[2 * mi.t + 0];
					vertexData.tv = mesh->texcoords[2 * mi.t + 1];
				}

				if (mi.n)
				{
					vertexData.nx = mesh->normals[3 * mi.n + 0];
					vertexData.ny = mesh->normals[3 * mi.n + 1];
					vertexData.nz = mesh->normals[3 * mi.n + 2];
				}
				idx++;
			}
		}
	}

	// TODO:
	// Need to use meshoptimizer in here 
	// instead of creating this massive index buffer which is not good.
	// (need to reduce the amount of vertex data and create a proper index buffer)
	for (uint32_t i = 0; i < mesh->index_count; ++i)
	{
		outMesh.indices[i] = i;
	}

	fast_obj_destroy(mesh);

	return true;
}



struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceAddress gpuAddress;
	void* data;
	uint32_t size;
};


struct Swapchain
{
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageviews;

	uint32_t width;
	uint32_t height;
};

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
	void CreateSwapchain();
	//
	//void createRenderpass();
	//
	void createPersistentlyMappedBuffer(Buffer& outBuffer, uint32_t size, VkBufferUsageFlags usageFlags);
	void createBuffer(Buffer& outBuffer, uint32_t size, VkBufferUsageFlags usageFlags, const void* data = nullptr, bool bPersistentlyMapped = false);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void DestroyBuffer(Buffer& Buffer);
	uint32_t selectMemoryType(const uint32_t memoryTypeBits, VkMemoryPropertyFlags flags);
	
	


	uint32_t getGraphicsQueueFamily();

	void RegisterDebugCallback();

	static VkImageMemoryBarrier ImageBarrier(VkImage Image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkShaderModule loadShader(const char* shaderPath) const;

	VkCommandPool CreateCommandPool();
};


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
	if (!loadMesh(testMesh, "assets/models/kitten.obj"))
	{
		// assert for now, hard crash is better to find this
		assert(!"");
	}

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
			}
		}

		// Main rendering loop for now
		uint32_t ImageIndex = 0;
		VK_CHECK(vkAcquireNextImageKHR(Device, swapchain.swapchain, ~0ull, AquireSemaphone, VK_NULL_HANDLE, &ImageIndex));

		VK_CHECK(vkResetCommandPool(Device, CommandPool, 0))


			VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;


		VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

		const VkImageMemoryBarrier RenderBeginBarrier = ImageBarrier(swapchain.images[ImageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &RenderBeginBarrier);


		constexpr VkClearColorValue ClearColorValue = { 27.0f / 255.0f, 3.0f / 255.0f, 3.0f / 255.0f, 1.0f };
		VkClearValue ClearValue = { ClearColorValue };

        VkRenderingAttachmentInfo ColorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        ColorAttachment.imageView = swapchain.imageviews[ImageIndex];
        ColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        ColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        ColorAttachment.clearValue = ClearValue;

        VkRenderingInfo RenderingInfo = { VK_STRUCTURE_TYPE_RENDERING_INFO };
        RenderingInfo.renderArea.extent = { swapchain.width, swapchain.height };
        RenderingInfo.layerCount = 1;
        RenderingInfo.colorAttachmentCount = 1;
        RenderingInfo.pColorAttachments = &ColorAttachment;

        vkCmdBeginRendering(CommandBuffer, &RenderingInfo);

		VkViewport Viewport = { 0, float(swapchain.height), float(swapchain.width), -float(swapchain.height), 0.0f, 1.0f };
		vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);

		VkRect2D Scissor = { 0, 0, (swapchain.width), (swapchain.height) };
		vkCmdSetScissor(CommandBuffer, 0, 1, &Scissor);

		vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_manager.TmpPipeline.Pipeline);

		vkCmdPushConstants(CommandBuffer,pipeline_manager.TmpPipeline.PipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(VkDeviceAddress),&vb.gpuAddress);

		vkCmdBindIndexBuffer(CommandBuffer, ib.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(CommandBuffer, static_cast<uint32_t>(testMesh.indices.size()), 1, 0, 0, 0);

		vkCmdEndRendering(CommandBuffer);

		const VkImageMemoryBarrier RenderEndBarrier = ImageBarrier(swapchain.images[ImageIndex], VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &RenderEndBarrier);

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

	// we need to enable this in order to get bindless support (which I plan to use as default) 
	VkPhysicalDeviceBufferDeviceAddressFeatures BDAFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
	BDAFeatures.bufferDeviceAddress = VK_TRUE; 

	// We do not want to deal with renderpasses nor framebuffers
	VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
	DynamicRenderingFeatures.dynamicRendering = VK_TRUE;
	DynamicRenderingFeatures.pNext = &BDAFeatures;
	
	//DeviceCreateInfo.enabledExtensionCount;
	//DeviceCreateInfo.ppEnabledExtensionNames;
	//DeviceCreateInfo.pEnabledFeatures;
	
	DeviceCreateInfo.pNext = &DynamicRenderingFeatures;

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
			copyBuffer(stagingBuffer.buffer, outBuffer.buffer, size);
			DestroyBuffer(stagingBuffer);
		}
	}
}

void EngineInstance::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandPool tempPool = CreateCommandPool();

	VkCommandBufferAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	AllocateInfo.commandPool = tempPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = 1;

	VkCommandBuffer CommandBuffer;
	VK_CHECK(vkAllocateCommandBuffers(Device, &AllocateInfo, &CommandBuffer));

	VkCommandBufferBeginInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(CommandBuffer, &BeginInfo));

	VkBufferCopy CopyRegion = {};
	CopyRegion.size = size;
	vkCmdCopyBuffer(CommandBuffer, srcBuffer, dstBuffer, 1, &CopyRegion);

	VK_CHECK(vkEndCommandBuffer(CommandBuffer));

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;

	VkQueue Queue;
	vkGetDeviceQueue(Device, FamilyIndex, 0, &Queue);
	VK_CHECK(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(Queue));

	vkFreeCommandBuffers(Device, tempPool, 1, &CommandBuffer);
	vkDestroyCommandPool(Device, tempPool, nullptr);
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
	Result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
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
