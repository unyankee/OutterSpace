#include <assert.h>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string>

#include <Volk/volk.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <extern/stb/stb_image.h>

#include <extern/meshoptimizer/extern/fast_obj.h>

#include "meshoptimizer.h"
#include "Common/Common.h"
#include "src/PipelineManager.h"
#include "src/Camera.h"
#include "src/Mesh.h"
#include "src/GpuResources.h"
#include "src/ResourceManager.h"
#include "src/Pipeline.h"
#include "src/Actor.h"
#include "src/EditorLayer.h"
#include <imgui.h>


const uint32_t StartupWidthResolution = 1920;
const uint32_t StartupHeightResolution = 1080;

using namespace ToyEngine;

struct GpuCameraData
{
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

class EngineInstance
{
public:
    VkInstance Instance;
    std::vector<VkPhysicalDevice> PhysicalDevices;
    VkPhysicalDevice PhysicalDevice;
    GLFWwindow* window;
    VkDevice Device;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR SurfaceCaps;
    VkSurfaceFormatKHR surfaceFormat;
    uint32_t FamilyIndex = 0;
    uint32_t swapchainImagesCount = 0;

    Swapchain swapchain;
    RenderTargetHandle DepthTexture;

    VkPhysicalDeviceMemoryProperties PhysicalMemoryProperties;
    VkDebugReportCallbackEXT DebugCallback;

    GpuContext gpuContext;
    PipelineManager pipeline_manager;
    ResourceManager resourceManager;
    Camera camera;

    BufferHandle camera_buffer;

    std::vector<Pipeline*> pipelines;
    std::vector<Actor*> actors;
    EditorLayer editorLayer;

    void MainLoop();
    void InitInstance();
    void SelectPhysicalDevice();
    void CreateDevice();
    void CreateSurface();
    void GetSwapchainFormat();

    void CreateSwapchain();
    void CreateDepthTexture();
    uint32_t selectMemoryType(const uint32_t memoryTypeBits, VkMemoryPropertyFlags flags);
    uint32_t getGraphicsQueueFamily();
    void RegisterDebugCallback();

    static VkImageMemoryBarrier ImageBarrier(VkImage Image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                                             VkImageLayout oldLayout, VkImageLayout newLayout);

};

VkBool32 DebugReporterVulkan(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object,
                             size_t location,
                             int32_t messageCode,
                             const char* pLayerPrefix,
                             const char* pMessage,
                             void* pUserData)
{
    const char* Type = (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
                           ? "ERROR"
                           : (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
                           ? "WARNING"
                           : "INFO";
    printf("%s : %s \n", Type, pMessage);
    return VK_FALSE;
};

void EngineInstance::RegisterDebugCallback()
{
    VkDebugReportCallbackCreateInfoEXT CreateInfo = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT};
    CreateInfo.pfnCallback = &DebugReporterVulkan;
    CreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    VK_CHECK(vkCreateDebugReportCallbackEXT(Instance, &CreateInfo, 0, &DebugCallback));
}

VkImageMemoryBarrier EngineInstance::ImageBarrier(VkImage Image, VkAccessFlags srcAccessMask,
                                                  VkAccessFlags dstAccessMask,
                                                  VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier Result = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    Result.srcAccessMask = srcAccessMask;
    Result.dstAccessMask = dstAccessMask;
    Result.oldLayout = oldLayout;
    Result.newLayout = newLayout;
    Result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Result.image = Image;
    Result.subresourceRange.aspectMask = (dstAccessMask == VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
                                             ? VK_IMAGE_ASPECT_DEPTH_BIT
                                             : VK_IMAGE_ASPECT_COLOR_BIT;
    Result.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    Result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    return Result;
}

void EngineInstance::InitInstance()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    volkInitialize();

    VkApplicationInfo ApplicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    ApplicationInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo InstanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    InstanceInfo.pApplicationInfo = &ApplicationInfo;

    const char* Extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };
    InstanceInfo.ppEnabledExtensionNames = Extensions;
    InstanceInfo.enabledExtensionCount = ARRAY_SIZE(Extensions);

#ifdef _DEBUG
    const char* DebugLayers[] = {"VK_LAYER_KHRONOS_validation"};
    InstanceInfo.ppEnabledLayerNames = DebugLayers;
    InstanceInfo.enabledLayerCount = ARRAY_SIZE(DebugLayers);
#endif

    VK_CHECK(vkCreateInstance(&InstanceInfo, nullptr, &Instance));
    volkLoadInstance(Instance);
    RegisterDebugCallback();

    uint32_t DeviceCount = 0;
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr);
    PhysicalDevices.resize(DeviceCount);
    vkEnumeratePhysicalDevices(Instance, &DeviceCount, PhysicalDevices.data());

    window = glfwCreateWindow(StartupWidthResolution, StartupHeightResolution, "ToyEngine", 0, 0);

    SelectPhysicalDevice();
    getGraphicsQueueFamily();
    CreateDevice();
    volkLoadDevice(Device);

    gpuContext.m_device = Device;
    gpuContext.m_physicalDevice = PhysicalDevice;
    gpuContext.m_memoryProperties = PhysicalMemoryProperties;
    vkGetDeviceQueue(Device, FamilyIndex, 0, &gpuContext.m_graphicsQueue);
    gpuContext.m_graphicsFamilyIndex = FamilyIndex;

    resourceManager.init(gpuContext);
    gpuContext.m_commandPool = resourceManager.createCommandPool(FamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    pipeline_manager.init(Device);

    camera.setPerspective(70.f, (float)StartupWidthResolution / (float)StartupHeightResolution, 0.1f, 1000.f);
    camera.update();

    camera_buffer = resourceManager.createBuffer(sizeof(GpuCameraData),
                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CreateSurface();
    GetSwapchainFormat();
    CreateSwapchain();

    editorLayer.init(gpuContext, &resourceManager, window, surfaceFormat.format);
}

void EngineInstance::SelectPhysicalDevice()
{
    uint32_t DeviceCandidate = 0;
    for (uint32_t i = 0; i < PhysicalDevices.size(); i++)
    {
        VkPhysicalDeviceProperties Properties;
        vkGetPhysicalDeviceProperties(PhysicalDevices[i], &Properties);
        if (Properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            DeviceCandidate = i;
            break;
        }
    }
    PhysicalDevice = PhysicalDevices[DeviceCandidate];
    vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalMemoryProperties);
}

void EngineInstance::CreateDevice()
{
    constexpr float QueuePriorities[] = {1.0f};
    VkDeviceQueueCreateInfo DeviceQueueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    DeviceQueueCreateInfo.queueFamilyIndex = FamilyIndex;
    DeviceQueueCreateInfo.queueCount = 1;
    DeviceQueueCreateInfo.pQueuePriorities = QueuePriorities;

    const char* Extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MESH_SHADER_EXTENSION_NAME};
    VkDeviceCreateInfo DeviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    DeviceCreateInfo.queueCreateInfoCount = 1;
    DeviceCreateInfo.pQueueCreateInfos = &DeviceQueueCreateInfo;
    DeviceCreateInfo.ppEnabledExtensionNames = Extensions;
    DeviceCreateInfo.enabledExtensionCount = ARRAY_SIZE(Extensions);

    VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceDynamicRenderingFeatures DynamicRenderingFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES
    };
    DynamicRenderingFeatures.dynamicRendering = VK_TRUE;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    meshShaderFeatures.taskShader = VK_TRUE;
    meshShaderFeatures.meshShader = VK_TRUE;
    meshShaderFeatures.pNext = &features12;

    DynamicRenderingFeatures.pNext = &meshShaderFeatures;

    VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features2.features.shaderInt64 = VK_TRUE;
    features2.pNext = &DynamicRenderingFeatures;
    DeviceCreateInfo.pNext = &features2;

    VK_CHECK(vkCreateDevice(PhysicalDevice, &DeviceCreateInfo, nullptr, &Device));
}

void EngineInstance::CreateSurface()
{
    VkWin32SurfaceCreateInfoKHR WindowCreateInfo{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    WindowCreateInfo.hinstance = GetModuleHandle(0);
    WindowCreateInfo.hwnd = glfwGetWin32Window(window);
    vkCreateWin32SurfaceKHR(Instance, &WindowCreateInfo, nullptr, &surface);
}

void EngineInstance::GetSwapchainFormat()
{
    uint32_t DeviceSurfaceCount = 0;
    std::vector<VkSurfaceFormatKHR> SupportedFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &DeviceSurfaceCount, nullptr);
    SupportedFormats.resize(DeviceSurfaceCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PhysicalDevice, surface, &DeviceSurfaceCount, SupportedFormats.data());
    surfaceFormat = SupportedFormats[0];
    for (const auto& format : SupportedFormats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            surfaceFormat = format;
            break;
        }
    }
}

void EngineInstance::CreateDepthTexture()
{
    if (DepthTexture.isValid())
    {
        resourceManager.destroyRenderTarget(DepthTexture);
    }
    DepthTexture = resourceManager.createRenderTarget(swapchain.width, swapchain.height, VK_FORMAT_D32_SFLOAT,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void EngineInstance::CreateSwapchain()
{
    vkDeviceWaitIdle(Device);
    if (swapchain.swapchain != VK_NULL_HANDLE)
    {
        for (auto imageView : swapchain.imageviews) vkDestroyImageView(Device, imageView, nullptr);
    }

    int Width, Height;
    glfwGetFramebufferSize(window, &Width, &Height);

    VkSwapchainCreateInfoKHR SwapchainCreateInfo{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    SwapchainCreateInfo.surface = surface;
    SwapchainCreateInfo.minImageCount = 3;
    SwapchainCreateInfo.imageFormat = surfaceFormat.format;
    SwapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    SwapchainCreateInfo.imageExtent.width = Width;
    SwapchainCreateInfo.imageExtent.height = Height;
    SwapchainCreateInfo.imageArrayLayers = 1;
    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    SwapchainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    SwapchainCreateInfo.oldSwapchain = swapchain.swapchain;

    VkSwapchainKHR LocalSwapchain;
    VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, nullptr, &LocalSwapchain));
    if (swapchain.swapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(Device, swapchain.swapchain, nullptr);

    swapchain.swapchain = LocalSwapchain;
    vkGetSwapchainImagesKHR(Device, swapchain.swapchain, &swapchainImagesCount, nullptr);
    swapchain.images.resize(swapchainImagesCount);
    vkGetSwapchainImagesKHR(Device, swapchain.swapchain, &swapchainImagesCount, swapchain.images.data());

    swapchain.imageviews.resize(swapchainImagesCount);
    for (uint32_t i = 0; i < swapchainImagesCount; ++i)
    {
        VkImageViewCreateInfo ImageViewCreateInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ImageViewCreateInfo.image = swapchain.images[i];
        ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewCreateInfo.format = surfaceFormat.format;
        ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ImageViewCreateInfo.subresourceRange.levelCount = 1;
        ImageViewCreateInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(Device, &ImageViewCreateInfo, nullptr, &swapchain.imageviews[i]));
    }
    swapchain.width = Width;
    swapchain.height = Height;
    CreateDepthTexture();
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
    return -1;
}

void EngineInstance::MainLoop()
{
    VkSemaphore acquireSemaphore = resourceManager.createSemaphore();
    VkSemaphore submitSemaphore = resourceManager.createSemaphore();
    VkFence renderFence = resourceManager.createFence(true);

    VkCommandPool CommandPool = resourceManager.createCommandPool(FamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    VkCommandBufferAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    AllocateInfo.commandPool = CommandPool;
    AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    AllocateInfo.commandBufferCount = 1;
    VkCommandBuffer CommandBuffer;
    vkAllocateCommandBuffers(Device, &AllocateInfo, &CommandBuffer);

    VkShaderModule MeshTask = Pipeline::loadShader(Device, "Shaders/mesh.task.spv");
    VkShaderModule MeshMesh = Pipeline::loadShader(Device, "Shaders/mesh.mesh.spv");
    VkShaderModule MeshFs = Pipeline::loadShader(Device, "Shaders/mesh.frag.spv");

    PipelineConfig config{};
    config.m_taskShader = MeshTask;
    config.m_meshShader = MeshMesh;
    config.m_fragmentShader = MeshFs;
    config.m_colorFormat = surfaceFormat.format;
    config.m_useMeshShaders = true;

    Pipeline* mainPipeline = new Pipeline(config);
    mainPipeline->create(gpuContext, pipeline_manager.getGlobalDescriptorSetLayout());
    
    pipelines.push_back(mainPipeline);

    Mesh* testMesh = new Mesh();
    //testMesh->loadFromObj("assets/models/sponza.obj");
    testMesh->loadFromObj("assets/models/dragon.obj");

    TextureHandle texture = resourceManager.loadTexture("assets/models/Dragon_Bump_Col2.jpg");
    pipeline_manager.AddTextureToGlobalDescriptorSet(*resourceManager.getTexture(texture));

    BufferHandle vb = resourceManager.createBuffer((uint32_t)(testMesh->m_vertices.size() * sizeof(Vertex)),
              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testMesh->m_vertices.data());
    
    BufferHandle meshletBuffer = resourceManager.createBuffer((uint32_t)(testMesh->m_meshlets.size() * sizeof(Meshlet)),
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testMesh->m_meshlets.data());

    BufferHandle meshletVertexBuffer = resourceManager.createBuffer((uint32_t)(testMesh->m_meshletVertices.size() * sizeof(uint32_t)),
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testMesh->m_meshletVertices.data());

    BufferHandle meshletTriangleBuffer = resourceManager.createBuffer((uint32_t)(testMesh->m_meshletTriangles.size() * sizeof(uint32_t)),
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testMesh->m_meshletTriangles.data());

    Actor* dragonActor = new Actor(testMesh);
    dragonActor->registerPipeline(mainPipeline);
    actors.push_back(dragonActor);

    double lastFrame = glfwGetTime();
    while (!glfwWindowShouldClose(window))
    {
        double currentFrame = glfwGetTime();
        float deltaTime = (float)(currentFrame - lastFrame);
        lastFrame = currentFrame;
        glfwPollEvents();

        editorLayer.beginFrame();
        
        ImGui::Begin("Engine Stats");
        ImGui::Text("Delta Time: %.3f ms (%.1f FPS)", deltaTime * 1000.0f, 1.0f / deltaTime);
        ImGui::End();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            static double lastX = xpos, lastY = ypos;
            camera.processMouseMovement((float)(xpos - lastX), (float)(lastY - ypos));
            lastX = xpos;
            lastY = ypos;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera.processKeyboard(FORWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera.processKeyboard(BACKWARD, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                camera.processKeyboard(LEFT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                camera.processKeyboard(RIGHT, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
                camera.processKeyboard(UP, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                camera.processKeyboard(DOWN, deltaTime);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        int Width, Height;
        glfwGetWindowSize(window, &Width, &Height);
        if (swapchain.width != (uint32_t)Width || swapchain.height != (uint32_t)Height)
        {
            CreateSwapchain();
            camera.setPerspective(70.f, (float)Width / (float)Height, 0.1f, 1000.f);
        }

        // Update camera data on GPU
        camera.update();
        GpuCameraData camData;
        camData.view = camera.getViewMatrix();
        camData.proj = camera.getProjectionMatrix();
        Buffer* cameraBufferRef = resourceManager.getBuffer(camera_buffer);
        cameraBufferRef->copyDataToBuffer(&camData, sizeof(GpuCameraData));

        vkWaitForFences(Device, 1, &renderFence, VK_TRUE, ~0ull);
        vkResetFences(Device, 1, &renderFence);

        
        uint32_t ImageIndex;
        VkResult acquireResult = vkAcquireNextImageKHR(Device, swapchain.swapchain, ~0ull, acquireSemaphore, VK_NULL_HANDLE, &ImageIndex);
        
        vkResetCommandPool(Device, CommandPool, 0);

        VkCommandBufferBeginInfo BeginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        vkBeginCommandBuffer(CommandBuffer, &BeginInfo);

        RenderTarget* depthTexture = resourceManager.getRenderTarget(DepthTexture);
        VkImageMemoryBarrier barriers[] = {
            ImageBarrier(swapchain.images[ImageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
            ImageBarrier(depthTexture->m_image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        };
        vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                             0, 0, nullptr, 0, nullptr, 2, barriers);

        VkRenderingAttachmentInfo colorAttachment = {
            VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, nullptr, swapchain.imageviews[ImageIndex],
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_RESOLVE_MODE_NONE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE
        };
        colorAttachment.clearValue.color = {0.1f, 0.01f, 0.01f, 1.0f};
        VkRenderingAttachmentInfo depthAttachment = {
            VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, nullptr, depthTexture->m_view,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_RESOLVE_MODE_NONE, VK_NULL_HANDLE,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE
        };
        depthAttachment.clearValue.depthStencil = {1.0f, 0};

        VkRenderingInfo renderingInfo = {
            VK_STRUCTURE_TYPE_RENDERING_INFO, nullptr, 0, {{0, 0}, {swapchain.width, swapchain.height}}, 1, 0, 1,
            &colorAttachment, &depthAttachment, nullptr
        };
        vkCmdBeginRendering(CommandBuffer, &renderingInfo);

        VkViewport viewport = {0, (float)swapchain.height, (float)swapchain.width, -(float)swapchain.height, 0, 1};
        vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);
        VkRect2D scissor = {{0, 0}, {swapchain.width, swapchain.height}};
        vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

        Buffer* vertexBuffer = resourceManager.getBuffer(vb);
        Buffer* cameraBuffer = resourceManager.getBuffer(camera_buffer);
        Buffer* meshlets = resourceManager.getBuffer(meshletBuffer);
        Buffer* meshletVertices = resourceManager.getBuffer(meshletVertexBuffer);
        Buffer* meshletTriangles = resourceManager.getBuffer(meshletTriangleBuffer);
        Texture* mainTexture = resourceManager.getTexture(texture);

        for (auto* pipeline : pipelines)
        {
            pipeline->bind(CommandBuffer);
            // Right now using a single set, with 2 bindings (global textures and samplers)
            VkDescriptorSet set = pipeline_manager.getGlobalDescriptorSet();
            // This is the set with the global pool of textures and samplers
            vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, &set,0, nullptr);
            
            for (auto* actor : actors)
            {
                if (actor->hasPipeline(pipeline))
                {
                    // Using pointer buffers to write the gpu address of the needed buffers / also pushing the texture + sampler needed
                    DefaultPipelineLayout push = {vertexBuffer->m_gpuAddress, cameraBuffer->m_gpuAddress,
                                                  meshlets->m_gpuAddress, meshletVertices->m_gpuAddress,
                                                  meshletTriangles->m_gpuAddress, mainTexture->m_bindlessIndex, 0};
                    
                    vkCmdPushConstants(CommandBuffer, pipeline->getLayout(), pipeline->getPipelineStageMask(), 0,sizeof(DefaultPipelineLayout),&push);
                    
                    vkCmdDrawMeshTasksEXT(CommandBuffer, (uint32_t)actor->getMesh()->m_meshlets.size(), 1, 1);
                }
            }
        }
        
        editorLayer.render(CommandBuffer, swapchain.width, swapchain.height);
        vkCmdEndRendering(CommandBuffer);

        VkImageMemoryBarrier presentBarrier = ImageBarrier(swapchain.images[ImageIndex],
                                                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0,
                                                           VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        
        vkCmdPipelineBarrier(CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &presentBarrier);
        vkEndCommandBuffer(CommandBuffer);

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo = {
            VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 1, &acquireSemaphore, &waitStage, 1, &CommandBuffer, 1,
            &submitSemaphore
        };
        vkQueueSubmit(gpuContext.m_graphicsQueue, 1, &submitInfo, renderFence);
        
        VkPresentInfoKHR presentInfo = {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr, 1, &submitSemaphore, 1, &swapchain.swapchain, &ImageIndex
        };
        vkQueuePresentKHR(gpuContext.m_graphicsQueue, &presentInfo);
    }

    editorLayer.destroy();

    if (Device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(Device);
    }

    resourceManager.cleanup();
}

int main()
{
    EngineInstance engine;
    engine.InitInstance();
    engine.MainLoop();
    return 0;
}
