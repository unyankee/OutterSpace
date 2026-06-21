#include <assert.h>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string>

#include <Volk/volk.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <extern/stb/stb_image.h>
#include "Common/Common.h"
#include "src/PipelineManager.h"
#include "src/Camera.h"
#include "src/Mesh.h"
#include "src/GpuResources.h"
#include "src/ResourceManager.h"
#include "src/Pipeline.h"
#include "src/Actor.h"
#include "src/Scene.h"
#include "src/Pass.h"
#include "src/PassExecutor.h"
#include "src/EditorLayer.h"
#include <imgui.h>


constexpr uint32_t MaxTransformsPerScene = 1 << 18;
constexpr uint32_t StartupWidthResolution = 1920;
constexpr uint32_t StartupHeightResolution = 1080;

using namespace ToyEngine;

struct GpuCameraData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 eyePos;
    float padding;
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
    RenderTargetHandle ColorTextureHandle;
    RenderTargetHandle DepthTextureHandle;

    VkPhysicalDeviceMemoryProperties PhysicalMemoryProperties;
    VkDebugReportCallbackEXT DebugCallback;

    GpuContext gpuContext;
    PipelineManager pipeline_manager;
    ResourceManager resourceManager;
    Camera camera;

    BufferHandle cameraBufferHandle;
    BufferHandle TransformBufferHandle;

    Scene scene;
    EditorLayer editorLayer;

    // Naive manual implementation for now 
    PassExecutor passExecutor;
    
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

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    EngineInstance* instance = (EngineInstance*)glfwGetWindowUserPointer(window);
    instance->camera.processMouseScroll((float)yoffset);
}

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
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, ScrollCallback);

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

    camera.setPerspective(70.f, (float)StartupWidthResolution / (float)StartupHeightResolution);
    camera.update();

    cameraBufferHandle = resourceManager.createBuffer(sizeof(GpuCameraData),
                                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Not the best, since this one is mapped to cpu, so first will got it working,
    // then will update with a gpu only buffer
    TransformBufferHandle = resourceManager.createBuffer(sizeof(Transform) * MaxTransformsPerScene,
                                                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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

    VkPhysicalDeviceVulkan13Features features13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.synchronization2 = VK_TRUE;
    features13.dynamicRendering = VK_TRUE;
    features13.maintenance4 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.timelineSemaphore = VK_TRUE;
    features12.drawIndirectCount = VK_TRUE;
    features12.pNext = &features13;

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT
    };
    meshShaderFeatures.taskShader = VK_TRUE;
    meshShaderFeatures.meshShader = VK_TRUE;
    meshShaderFeatures.pNext = &features12;

    VkPhysicalDeviceFeatures2 features2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features2.features.shaderInt64 = VK_TRUE;
    features2.pNext = &meshShaderFeatures;
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
    if (DepthTextureHandle.isValid())
    {
        resourceManager.destroyRenderTarget(DepthTextureHandle);
    }
    DepthTextureHandle = resourceManager.createRenderTarget(swapchain.width, swapchain.height, VK_FORMAT_D32_SFLOAT,
                                                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                            VK_IMAGE_ASPECT_DEPTH_BIT);
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
    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    SwapchainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    SwapchainCreateInfo.oldSwapchain = swapchain.swapchain;

    VkSwapchainKHR LocalSwapchain;
    VK_CHECK(vkCreateSwapchainKHR(Device, &SwapchainCreateInfo, nullptr, &LocalSwapchain));
    if (swapchain.swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(Device, swapchain.swapchain, nullptr);
    }

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

    if (ColorTextureHandle.isValid())
    {
        resourceManager.destroyRenderTarget(ColorTextureHandle);
    }
    ColorTextureHandle = resourceManager.createRenderTarget(swapchain.width, swapchain.height, surfaceFormat.format,
                                                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                            VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
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
    const uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    VkSemaphore acquireSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore submitSemaphores[MAX_FRAMES_IN_FLIGHT];
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        acquireSemaphores[i] = resourceManager.createBinarySemaphore();
        submitSemaphores[i] = resourceManager.createBinarySemaphore();
    }

    VkSemaphore timelineSemaphore = resourceManager.createSemaphore(0);
    uint64_t timelineValue = 0;

    VkCommandPool commandPools[MAX_FRAMES_IN_FLIGHT];
    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        commandPools[i] = resourceManager.createCommandPool(FamilyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
        VkCommandBufferAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        AllocateInfo.commandPool = commandPools[i];
        AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        AllocateInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(Device, &AllocateInfo, &commandBuffers[i]);
    }

    VkShaderModule MeshTask = Pipeline::loadShader(Device, "Shaders/mesh.task.spv");
    VkShaderModule MeshMesh = Pipeline::loadShader(Device, "Shaders/mesh.mesh.spv");
    VkShaderModule MeshFs = Pipeline::loadShader(Device, "Shaders/mesh.frag.spv");

    
    Mesh* testMesh = new Mesh();
    testMesh->loadFromObj("assets/models/kitten.obj");

    TextureHandle texture = resourceManager.loadTexture("assets/models/Dragon_Bump_Col2.jpg");
    pipeline_manager.AddTextureToGlobalDescriptorSet(*resourceManager.getTexture(texture));

    BufferHandle vb = resourceManager.createBuffer((uint32_t)(testMesh->m_vertices.size() * sizeof(Vertex)),
                                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testMesh->m_vertices.data());

    BufferHandle meshletBuffer = resourceManager.createBuffer((uint32_t)(testMesh->m_meshlets.size() * sizeof(Meshlet)),
                                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                              testMesh->m_meshlets.data());

    BufferHandle meshletVertexBuffer = resourceManager.createBuffer(
        (uint32_t)(testMesh->m_meshletVertices.size() * sizeof(uint32_t)),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testMesh->m_meshletVertices.data());

    BufferHandle meshletTriangleBuffer = resourceManager.createBuffer(
        (uint32_t)(testMesh->m_meshletTriangles.size() * sizeof(uint32_t)),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testMesh->m_meshletTriangles.data());

    for(uint32_t i = 0; i < 10; ++i)
    {
        Actor dragonActor = scene.createActor();
        dragonActor.addComponent<Mesh*>(testMesh);
        
        Transform& transformData = scene.transformSystem.getTransform(dragonActor);
        transformData.m_scale = glm::vec4(60.0, 60.0, 60.0, 1.0);
        transformData.m_position = glm::vec4(0.0 + i * 25, 0.0, 0.0, 1.0);
    }

    // Main pass config
    PipelineConfig config{};
    config.m_taskShader = MeshTask;
    config.m_meshShader = MeshMesh;
    config.m_fragmentShader = MeshFs;
    config.m_colorFormat = surfaceFormat.format;
    config.m_useMeshShaders = true;
    
    VkPushConstantRange mainPushConstantRange{};
    mainPushConstantRange.stageFlags = VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT;
    mainPushConstantRange.offset = 0;
    mainPushConstantRange.size = sizeof(DefaultPipelineLayout);

    Pass mainPass;
    mainPass.name = "MainForwardPass";
    mainPass.pipeline = resourceManager.createPipeline(config, pipeline_manager.getGlobalDescriptorSetLayout(), { mainPushConstantRange });
    mainPass.execute = [vb, meshletBuffer, meshletVertexBuffer, meshletTriangleBuffer, CameraBufferHandle = cameraBufferHandle, TransformBufferHandle = TransformBufferHandle, texture](
        VkCommandBuffer cmd, const Pass& pass, PassContext& ctx)
        {
            // obviously not ideal, we could multidraw indirect if mesh is the same
            // but this is not on that stage yet 
            auto view = ctx.scene.getRegistry().view<Mesh*, TransformIndex>();            
            for (const auto& [entity, mesh, transformIndex]  : view.each())
            {
                uint32_t meshletCount = (uint32_t)mesh->m_meshlets.size();

                Buffer* vertexBuffer = ctx.resourceManager.getBuffer(vb);
                Buffer* cameraBuffer = ctx.resourceManager.getBuffer(CameraBufferHandle);
                Buffer* meshlets = ctx.resourceManager.getBuffer(meshletBuffer);
                Buffer* Transform = ctx.resourceManager.getBuffer(TransformBufferHandle);
                Buffer* meshletVertices = ctx.resourceManager.getBuffer(meshletVertexBuffer);
                Buffer* meshletTriangles = ctx.resourceManager.getBuffer(meshletTriangleBuffer);
                Texture* mainTexture = ctx.resourceManager.getTexture(texture);

                DefaultPipelineLayout push = {
                    vertexBuffer->m_gpuAddress, cameraBuffer->m_gpuAddress,
                    meshlets->m_gpuAddress, meshletVertices->m_gpuAddress,
                    meshletTriangles->m_gpuAddress, Transform->m_gpuAddress, mainTexture->m_bindlessIndex, 0,
                    meshletCount, transformIndex.index
                };

                Pipeline* pipeline = ctx.resourceManager.getPipeline(pass.pipeline);
                vkCmdPushConstants(cmd, pipeline->getLayout(), pipeline->getPipelineStageMask(), 0,
                                   sizeof(DefaultPipelineLayout), &push);

                // The idea is to get a list of drawcommands at this stage automatically?
                // It should be an isolated chunk of gpu commands that can be
                // executed on parallel and the whole thing should live in this lambda
                //
                // Input should be some list of already pre filtered meshes + transforms?
                // Push constant values, we might get some other variable to fill
                // later then will assign stuff to the needed DefaultPipelineLayout, maybe?
                // vkCmdDrawMeshTasksIndirectCountEXT();
                
                
                vkCmdDrawMeshTasksEXT(cmd, divideAndRoundUp(meshletCount, 32), 1, 1);
            }
        };

    Pass editorPass;
    editorPass.name = "EditorPass";
    editorPass.pipeline = editorLayer.GetPipeline();
    editorPass.descriptorSets = editorLayer.GetDescriptorSets();
    editorPass.execute = [&editorLayer = editorLayer, &swapchain = swapchain](
        VkCommandBuffer cmd, const Pass& pass, PassContext& ctx)
        {
            editorLayer.render(cmd, swapchain.width, swapchain.height);
        };

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
            break;
        }

        static bool bWasMousePressed = true;
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            static double lastX = xpos, lastY = ypos;
            if (bWasMousePressed)
            {
                lastX = xpos;
                lastY = ypos;
                bWasMousePressed = false;
            }
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
            bWasMousePressed = true;
        }

        int Width, Height;
        glfwGetWindowSize(window, &Width, &Height);
        if (swapchain.width != (uint32_t)Width || swapchain.height != (uint32_t)Height)
        {
            CreateSwapchain();
            camera.setPerspective(70.f, (float)Width / (float)Height);
        }

        camera.update();
        GpuCameraData camData;
        camData.view = camera.getViewMatrix();
        camData.proj = camera.getProjectionMatrix();
        camData.eyePos = camera.getPosition();
        Buffer* cameraBufferRef = resourceManager.getBuffer(cameraBufferHandle);
        cameraBufferRef->copyDataToBuffer(&camData, sizeof(GpuCameraData));
        
        // Iterates and update all transform data
        // non-optimal at all, no need to do every frame and a lot of reasons, but... shortcuts
        scene.transformSystem.update();
        Buffer* TranformBufferRef = resourceManager.getBuffer(TransformBufferHandle);
        TranformBufferRef->copyDataToBuffer(scene.transformSystem.TransformsData.data(), sizeof(Transform) * scene.transformSystem.TransformsData.size());

        
        uint32_t frameIndex = timelineValue % MAX_FRAMES_IN_FLIGHT;
        uint64_t waitValue = timelineValue >= MAX_FRAMES_IN_FLIGHT ? timelineValue - MAX_FRAMES_IN_FLIGHT + 1 : 0;

        if (waitValue > 0)
        {
            VkSemaphoreWaitInfo waitInfo{VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
            waitInfo.semaphoreCount = 1;
            waitInfo.pSemaphores = &timelineSemaphore;
            waitInfo.pValues = &waitValue;
            vkWaitSemaphores(Device, &waitInfo, ~0ull);
        }

        VkSemaphore acquireSemaphore = acquireSemaphores[frameIndex];
        VkSemaphore submitSemaphore = submitSemaphores[frameIndex];
        VkCommandPool currentCommandPool = commandPools[frameIndex];
        VkCommandBuffer currentCommandBuffer = commandBuffers[frameIndex];

        uint32_t ImageIndex;
        VkResult acquireResult = vkAcquireNextImageKHR(Device, swapchain.swapchain, ~0ull, acquireSemaphore,
                                                       VK_NULL_HANDLE, &ImageIndex);

        vkResetCommandPool(Device, currentCommandPool, 0);

        VkCommandBufferBeginInfo BeginInfo = {
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        vkBeginCommandBuffer(currentCommandBuffer, &BeginInfo);

        RenderTarget* colorTexture = resourceManager.getRenderTarget(ColorTextureHandle);
        RenderTarget* depthTexture = resourceManager.getRenderTarget(DepthTextureHandle);

        VkImageMemoryBarrier barriers[] = {
            ImageBarrier(colorTexture->m_image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
            ImageBarrier(depthTexture->m_image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        };
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                             0, 0, nullptr, 0, nullptr, 2, barriers);

        VkViewport viewport = {0, (float)swapchain.height, (float)swapchain.width, -(float)swapchain.height, 0, 1};
        vkCmdSetViewport(currentCommandBuffer, 0, 1, &viewport);
        VkRect2D scissor = {{0, 0}, {swapchain.width, swapchain.height}};
        vkCmdSetScissor(currentCommandBuffer, 0, 1, &scissor);

        RenderBatch frameBatch;
        frameBatch.name = "MainFrameBatch";
        frameBatch.colorAttachments = {
            {
                ColorTextureHandle, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
                {{0.1f, 0.01f, 0.01f, 1.0f}}
            }
        };
        frameBatch.depthAttachment = {
            DepthTextureHandle, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, {0.0f, 0} // reverse depth buffer, clears to 0
        };
        frameBatch.useDepth = true;
        frameBatch.passes.push_back(mainPass);
        frameBatch.passes.push_back(editorPass);

        BufferHandle camBuffer = cameraBufferHandle;
        PassContext ctx = {scene, resourceManager, pipeline_manager};
        passExecutor.execute(currentCommandBuffer, frameBatch, ctx);


        
        VkImageCopy copyRegion{};
        copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.srcOffset = {0, 0, 0};
        copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copyRegion.dstOffset = {0, 0, 0};
        copyRegion.extent = {swapchain.width, swapchain.height, 1};

        VkImageMemoryBarrier srcBarrier = ImageBarrier(colorTexture->m_image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                       VK_ACCESS_TRANSFER_READ_BIT,
                                                       VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkImageMemoryBarrier dstBarrier = ImageBarrier(swapchain.images[ImageIndex], 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                                                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &srcBarrier);
        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &dstBarrier);

        vkCmdCopyImage(currentCommandBuffer, colorTexture->m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       swapchain.images[ImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier presentBarrier = ImageBarrier(swapchain.images[ImageIndex], VK_ACCESS_TRANSFER_WRITE_BIT,
                                                           0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                           VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        vkCmdPipelineBarrier(currentCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

        vkEndCommandBuffer(currentCommandBuffer);

        timelineValue++;

        VkSemaphoreSubmitInfo waitSemaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitSemaphoreInfo.semaphore = acquireSemaphore;
        waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkCommandBufferSubmitInfo cmdBufferInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
        cmdBufferInfo.commandBuffer = currentCommandBuffer;

        VkSemaphoreSubmitInfo signalSemaphores[2] = {};
        signalSemaphores[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphores[0].semaphore = submitSemaphore;
        signalSemaphores[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        signalSemaphores[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signalSemaphores[1].semaphore = timelineSemaphore;
        signalSemaphores[1].value = timelineValue;
        signalSemaphores[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        VkSubmitInfo2 submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &cmdBufferInfo;
        submitInfo.signalSemaphoreInfoCount = 2;
        submitInfo.pSignalSemaphoreInfos = signalSemaphores;

        vkQueueSubmit2(gpuContext.m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

        VkPresentInfoKHR presentInfo = {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, nullptr, 1, &submitSemaphore, 1, &swapchain.swapchain, &ImageIndex
        };
        vkQueuePresentKHR(gpuContext.m_graphicsQueue, &presentInfo);
    }


    if (Device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(Device);
    }

    editorLayer.destroy();
    resourceManager.cleanup();
}

int main()
{
    EngineInstance engine;
    engine.InitInstance();
    engine.MainLoop();
    return 0;
}
