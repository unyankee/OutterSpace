#include "engine.h"

#include "..\include\vulkan_debug.h"
#include "..\include\vulkan_initializers.h"
#include <iostream>

#include <imgui.h>
#include <ppl.h>

#include <array>

#include <filesystem>

//--------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------//

Engine::Engine() : m_swap_chain(*this), m_queue_manager(*this)
{
    start_running_time_stamp = std::chrono::steady_clock::now();
    m_swap_chain.set_size(1920, 1080);
    m_camera.init();

    m_shadow_camera.init();
}

void Engine::init()
{
    // could not get windows entry point to work, so retrieve this data in here
    // is ugly, but will do it by now
    HINSTANCE_windowInstance = GetModuleHandle(NULL);
    init_device();
    setup_window();

    {
        Mesh cube;
        for (auto&& vertex : cube_vertices)
        {
            cube.m_vertex.push_back(vertex);
        }
        for (auto&& idx : cube_indices)
        {
            cube.m_indices.push_back(idx);
        }

        m_mesh_system.register_new_mesh(cube);
        // m_mesh_system.register_new_mesh(triangle);

        const int32_t cubes_x     = 1;
        const int32_t cubes_y     = 1;
        const int32_t cubes_z     = 1;
        const int32_t offset_size = 3;

        m_test_entity.push_back(create_entity());
        m_test_entity.back().add_component<Transform>().set_pos(glm::vec3(0.0f, -5.0f, 0.0f));
        m_test_entity.back().get_component<Transform>().set_ori(glm::vec3(0.0f, 0.0f, 0.0f));
        m_test_entity.back().get_component<Transform>().set_sca(glm::vec3(100.0f, 1.0f, 100.0f));
        m_test_entity.back().add_component<Mesh_renderer>().set_mesh(cube);

        // TEST PLACE !!

        for (int32_t x = -cubes_x / 2; x < cubes_x / 2; ++x)
        {
            for (int32_t y = -cubes_y / 2; y < cubes_y / 2; ++y)
            {
                for (int32_t z = -cubes_z / 2; z < cubes_z / 2; ++z)
                {
                    m_test_entity.push_back(create_entity());
                    Transform& transform = m_test_entity.back().add_component<Transform>();
                    transform.set_pos(
                        glm::vec3(
                            cubes_x + (offset_size * x), cubes_y + (offset_size * y),
                            // cubes_z + (offset_size * z)) + glm::vec3(-170, 30.0, -cubes_x / 2));
                            cubes_z + (offset_size * z)) +
                        glm::vec3(0, 0.0, 0));

                    transform.set_ori(glm::vec3(1.0, 1.0f, 1.0f));
                    transform.set_sca(glm::vec3(1.0, 1.0f, 1.0f));
                    m_test_entity.back().add_component<Mesh_renderer>().set_mesh(cube);
                }
            }
        }

        for (int32_t x = 1; x < 10; ++x)
        {
            m_test_entity.push_back(create_entity());
            Transform& transform = m_test_entity.back().add_component<Transform>();
            transform.set_pos(glm::vec3(0.0f + (x * 2.0f), 0.0f, 0.0f));
            transform.set_ori(glm::vec3(0.0f, 0.0f, 0.0f));
            transform.set_sca(glm::vec3(0.5f, 0.5f, 0.5f));
            m_test_entity.back().add_component<Mesh_renderer>().set_mesh(cube);
        }

        for (int32_t x = 1; x < 10; ++x)
        {
            m_test_entity.push_back(create_entity());
            Transform& transform = m_test_entity.back().add_component<Transform>();
            transform.set_pos(glm::vec3(0.0f, 0.0f + (x * 2.0f), 0.0f));
            transform.set_ori(glm::vec3(0.0f, 0.0f, 0.0f));
            transform.set_sca(glm::vec3(0.5f, 0.5f, 0.5f));
            m_test_entity.back().add_component<Mesh_renderer>().set_mesh(cube);
        }

        for (int32_t x = 1; x < 10; ++x)
        {
            m_test_entity.push_back(create_entity());
            Transform& transform = m_test_entity.back().add_component<Transform>();
            transform.set_pos(glm::vec3(0.0f, 0.0f, 0.0f + (x * 2.0f)));
            transform.set_ori(glm::vec3(0.0f, 0.0f, 0.0f));
            transform.set_sca(glm::vec3(0.5f, 0.5f, 0.5f));
            m_test_entity.back().add_component<Mesh_renderer>().set_mesh(cube);
        }
        //-----------------------------------------//

        //-----------------------------------------//
    }

    {
        m_swap_chain.init(m_device.m_vk_instance, m_device.m_vulkan_device);
        m_swap_chain.create_swapchain(m_device.m_settings.m_enable_vsync);
    }

    {
        m_graphics_queue             = m_queue_manager.create_new_queue(QUEUE_TYPE::GRAPHICS);
        m_graphics_command_pool_data = m_queue_manager.create_command_pool(m_graphics_queue.m_queue_type, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        m_graphics_command_buffers_data =
            m_queue_manager.create_command_buffer(m_swap_chain.image_count(), VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_graphics_command_pool_data);

        m_transfer_queue             = m_queue_manager.create_new_queue(QUEUE_TYPE::TRANFER);
        m_transfer_command_pool_data = m_queue_manager.create_command_pool(m_transfer_queue.m_queue_type, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        m_transfer_command_buffers_data =
            m_queue_manager.create_command_buffer(m_swap_chain.image_count(), VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_transfer_command_pool_data);
    }

    {
        m_graph.reset(new Graph(*this));

        m_graph->m_steps.push_back(&m_main_step);

        {
            TextureInfo info = {};
            info.imageType   = VK_IMAGE_TYPE_2D;
            info.format      = VK_FORMAT_R8G8B8A8_UNORM;
            info.extent      = {m_swap_chain.width(), m_swap_chain.height(), 1};
            info.samples     = VK_SAMPLE_COUNT_1_BIT;
            info.usage       = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // another testing, sampling from this texture
            info.m_current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            info.attachmentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            // info.finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            info.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            info.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            info.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;

            m_texture_out_test = m_graph->m_resources.create_colour_attachment(info);

            info.format = m_device.m_depthFormat;
            info.usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            // info.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            m_rendergraph_depth_attachment = m_graph->m_resources.create_depth_attachment(info);

            info.extent = {m_swap_chain.width(), m_swap_chain.height(), 1};

            info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            // info.finalLayout                     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            m_directional_light_depth_attachment = m_graph->m_resources.create_depth_attachment(info);
        }
        {
            BufferInfo info;
            info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            info.size              = sizeof(m_scene_data_cpu);
            // NEEDS TO FIND A WAY TO CALL UPDATE RESOURCE, AND LOAD THE RIGHT DATA INTO THESE GUYS !!
            m_global_scene_data = m_graph->m_resources.create_ubo(info);
        }
    }

    {
        prepare_vertex_buffers();
        // prepare_uniform_ssbo_buffers();
    }

    // const ShaderConfig vertex1("main", "../engine/data/shaders/glsl/mesh/mesh.vert_bin.spv", VK_SHADER_STAGE_VERTEX_BIT);
    // const ShaderConfig fragment1("main", "../engine/data/shaders/glsl/mesh/mesh.frag_bin.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    //
    // std::vector<ShaderConfig> configs1;
    // configs1.push_back(vertex1);
    // configs1.push_back(fragment1);
    // m_graph->task_description1 = m_graph->create_task_description(configs1);

    prepare();

    // Imgui needs to be converted to a rendergraph pass itself
    if (m_device.m_settings.enable_imgui == true)
    {
        // m_ui.prepare(this);
    }
}

void Engine::update()
{
    if (frameCounter == 0)
    {
        // TODO: This should not be in this level...
        for (uint32_t i = 0; i < m_graph->m_steps.size(); ++i)
        {
            m_graph->m_steps[i]->create_resources(*m_graph);
        }
    }
    if (prepared /*&& !IsIconic(window)*/)
    {
        update_frame();
    }
}

bool Engine::init_device()
{
    VkResult err;
    m_device.create_device_instance();
    // TODO: proper error management
    return true;
}

#define KEY_ESCAPE VK_ESCAPE
#define KEY_F1     VK_F1
#define KEY_F2     VK_F2
#define KEY_F3     VK_F3
#define KEY_F4     VK_F4
#define KEY_F5     VK_F5
#define KEY_W      0x57
#define KEY_A      0x41
#define KEY_S      0x53
#define KEY_D      0x44
#define KEY_P      0x50
#define KEY_Q      81
#define KEY_E      69
#define KEY_SPACE  0x20
#define KEY_KPADD  0x6B
#define KEY_KPSUB  0x6D
#define KEY_B      0x42
#define KEY_F      0x46
#define KEY_L      0x4C
#define KEY_N      0x4E
#define KEY_O      0x4F
#define KEY_T      0x54

void Engine::handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            prepared = false;
            DestroyWindow(hWnd);
            PostQuitMessage(0);
            break;
        case WM_PAINT: ValidateRect(m_HWND_window, NULL); break;
        case WM_KEYDOWN:
            switch (wParam)
            {
                case KEY_P:
                    // paused = !paused;
                    break;
                case KEY_F1:
                    // if (settings.overlay) {
                    //	UIOverlay.visible = !UIOverlay.visible;
                    // }
                    break;
                case KEY_ESCAPE: PostQuitMessage(0); break;
            }

            {
                switch (wParam)
                {
                    case KEY_W: m_camera.keys.forward = true; break;
                    case KEY_S: m_camera.keys.backward = true; break;
                    case KEY_A: m_camera.keys.left = true; break;
                    case KEY_D: m_camera.keys.right = true; break;
                    case KEY_Q: m_camera.keys.down = true; break;
                    case KEY_E: m_camera.keys.up = true; break;
                    case VK_SHIFT: m_camera.movementMultiplier = 2.5f; break;
                }
            }

            break;
        case WM_KEYUP:
            {
                switch (wParam)
                {
                    case KEY_W: m_camera.keys.forward = false; break;
                    case KEY_S: m_camera.keys.backward = false; break;
                    case KEY_A: m_camera.keys.left = false; break;
                    case KEY_D: m_camera.keys.right = false; break;
                    case KEY_Q: m_camera.keys.down = false; break;
                    case KEY_E: m_camera.keys.up = false; break;
                    case VK_SHIFT: m_camera.movementMultiplier = 1.0f; break;
                }
            }
            break;
        case WM_LBUTTONDOWN:
            m_camera.mousePos          = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
            m_camera.mouseButtons.left = true;
            break;
        case WM_RBUTTONDOWN:
            m_camera.mousePos           = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
            m_camera.mouseButtons.right = true;
            break;
        case WM_MBUTTONDOWN:
            m_camera.mousePos            = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
            m_camera.mouseButtons.middle = true;
            break;
        case WM_LBUTTONUP: m_camera.mouseButtons.left = false; break;
        case WM_RBUTTONUP: m_camera.mouseButtons.right = false; break;
        case WM_MBUTTONUP: m_camera.mouseButtons.middle = false; break;
        case WM_MOUSEWHEEL:
            {
                // short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                // camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
                // viewUpdated = true;
                break;
            }
        case WM_MOUSEMOVE:
            {
                m_camera.mouse_move(LOWORD(lParam), HIWORD(lParam));
                break;
            }
        case WM_SIZE:
            if (prepared && wParam != SIZE_MINIMIZED)
            {
                if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
                {
                    destWidth  = LOWORD(lParam);
                    destHeight = HIWORD(lParam);
                    window_resize();
                }
            }
            break;
        case WM_GETMINMAXINFO:
            {
                LPMINMAXINFO minMaxInfo      = (LPMINMAXINFO)lParam;
                minMaxInfo->ptMinTrackSize.x = 64;
                minMaxInfo->ptMinTrackSize.y = 64;
                break;
            }
        case WM_ENTERSIZEMOVE: resizing = true; break;
        case WM_EXITSIZEMOVE: resizing = false; break;
    }
}

void Engine::window_resize()
{
    if (!prepared)
    {
        return;
    }
    prepared = false;

    // Ensure all operations on the device have been finished before destroying resources
    vkDeviceWaitIdle(m_device.m_vulkan_device);

    // Recreate swap chain
    m_swap_chain.set_size(destWidth, destHeight);

    m_swap_chain.create_swapchain(m_device.m_settings.m_enable_vsync);

    // Recreate the frame buffers
    // vkDestroyImageView(m_device_abstraction.m_vulkan_device, m_depthStencil.view, nullptr);
    // vkDestroyImage(m_device_abstraction.m_vulkan_device, m_depthStencil.image, nullptr);
    // vkFreeMemory(m_device_abstraction.m_vulkan_device, m_depthStencil.mem, nullptr);
    // setup_depth_stencil();

    // m_swapchain_framebuffers.clean(m_device.m_vulkan_device);

    // setup_swapchain_framebuffer();

    if ((destWidth > 0.0f) && (destHeight > 0.0f))
    {
        if (m_device.m_settings.enable_imgui)
        {
            // m_ui.resize(destWidth, destHeight);
        }
    }

    // Command buffers need to be recreated as they may store
    // references to the recreated frame buffer
    // destroyCommandBuffers();
    // create_command_buffer();

    m_queue_manager.free_command_buffer(m_queue_manager.get_command_buffer(m_graphics_command_buffers_data));
    m_graphics_command_buffers_data =
        m_queue_manager.create_command_buffer(m_swap_chain.image_count(), VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_graphics_command_pool_data);

    vkDeviceWaitIdle(m_device.m_vulkan_device);

    prepared = true;
}

Engine*          global_engine;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (global_engine != NULL)
    {
        global_engine->handle_messages(hWnd, uMsg, wParam, lParam);
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void Engine::setup_window()
{

    global_engine = this;

    WNDCLASSEX wndClass;

    wndClass.cbSize        = sizeof(WNDCLASSEX);
    wndClass.style         = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = WndProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = HINSTANCE_windowInstance;
    wndClass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = "Outterspace";
    wndClass.hIconSm       = LoadIcon(NULL, IDI_WINLOGO);

    if (!RegisterClassEx(&wndClass))
    {
        std::cout << "Could not register m_HWND_window class!\n";
        fflush(stdout);
        exit(1);
    }

    int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (m_device.m_settings.m_fullscreen)
    {
        if ((m_swap_chain.width() != (uint32_t)screenWidth) && (m_swap_chain.height() != (uint32_t)screenHeight))
        {
            DEVMODE dmScreenSettings;
            memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
            dmScreenSettings.dmSize       = sizeof(dmScreenSettings);
            dmScreenSettings.dmPelsWidth  = m_swap_chain.width();
            dmScreenSettings.dmPelsHeight = m_swap_chain.height();
            dmScreenSettings.dmBitsPerPel = 32;
            dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            {
                if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to m_HWND_window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
                {
                    m_device.m_settings.m_fullscreen = false;
                }
                else
                {
                    // TODO: proper assert
                    return;
                }
            }
            screenWidth  = m_swap_chain.width();
            screenHeight = m_swap_chain.height();
        }
    }

    DWORD dwExStyle;
    DWORD dwStyle;

    if (m_device.m_settings.m_fullscreen)
    {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle   = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    }

    RECT windowRect;
    windowRect.left   = 0L;
    windowRect.top    = 0L;
    windowRect.right  = m_device.m_settings.m_fullscreen ? (long)screenWidth : (long)m_swap_chain.width();
    windowRect.bottom = m_device.m_settings.m_fullscreen ? (long)screenHeight : (long)m_swap_chain.height();

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    // std::string windowTitle = getWindowTitle();
    std::string windowTitle = "Outterspace";
    m_HWND_window           = CreateWindowEx(
                  0, "Outterspace", windowTitle.c_str(), dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL,
                  NULL, HINSTANCE_windowInstance, NULL);

    if (!m_device.m_settings.m_fullscreen)
    {
        // Center on screen
        uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
        uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
        SetWindowPos(m_HWND_window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }

    if (!m_HWND_window)
    {
        printf("Could not create m_HWND_window!\n");
        fflush(stdout);
        return;
    }

    ShowWindow(m_HWND_window, SW_SHOW);
    SetForegroundWindow(m_HWND_window);
    SetFocus(m_HWND_window);
}

void Engine::prepare()
{
    // if we are going to use debug markers, we need to init them in here
    if (m_device.m_settings.m_enableDebugMarkers)
    {
        vks::debugmarker::setup(m_device.m_vulkan_device);
    };

    vkCmdBeginRenderingKHR   = reinterpret_cast<PFN_vkCmdBeginRenderingKHR>(vkGetDeviceProcAddr(m_device.m_vulkan_device, "vkCmdBeginRenderingKHR"));
    vkCmdEndRenderingKHR     = reinterpret_cast<PFN_vkCmdEndRenderingKHR>(vkGetDeviceProcAddr(m_device.m_vulkan_device, "vkCmdEndRenderingKHR"));
    vkCmdPipelineBarrier2KHR = reinterpret_cast<PFN_vkCmdPipelineBarrier2KHR>(vkGetDeviceProcAddr(m_device.m_vulkan_device, "vkCmdPipelineBarrier2KHR"));

    // Engine needed stuff
    // setup_swapchain_framebuffer();
    create_synchronization_primitives();
    prepare_synchronization_primitives();

    // std::vector<VkFormatProperties> format_properties;
    // vkGetPhysicalDeviceFormatProperties(m_device.m_vulkan_device, );

    prepared = true;
}

void Engine::create_synchronization_primitives()
{
    // Wait fences to sync command buffer access
    VkFenceCreateInfo fenceCreateInfo = initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    m_waitFences.resize(m_swap_chain.image_count());
    for (auto& fence : m_waitFences)
    {
        vkCreateFence(m_device.m_vulkan_device, &fenceCreateInfo, nullptr, &fence);
    }
}

// This function is used to request a device memory type that supports all the property flags we request (e.g. device
// local, host visible) Upon success it will return the index of the memory type that fits our requested memory
// properties This is necessary as implementations can offer an arbitrary number of memory types with different memory
// properties. You can check http://vulkan.gpuinfo.org/ for details on different memory configurations
const uint32_t Engine::getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const
{
    // Iterate over all memory types available for the device used in this example
    for (uint32_t i = 0; i < m_device.m_device_memory_properties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((m_device.m_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        typeBits >>= 1;
    }

    throw "Could not find a suitable memory type!";
}

void Engine::prepare_synchronization_primitives()
{
    // Semaphores (Used for correct command ordering)
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext                 = nullptr;

    // Semaphore used to ensures that image presentation is complete before starting to submit again
    (vkCreateSemaphore(m_device.m_vulkan_device, &semaphoreCreateInfo, nullptr, &m_presentCompleteSemaphore));

    // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
    (vkCreateSemaphore(m_device.m_vulkan_device, &semaphoreCreateInfo, nullptr, &m_renderCompleteSemaphore));

    // Fences (Used to check draw command buffer completion)
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Create in signaled state so we don't wait on first render of each command buffer
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    m_waitFences.resize(m_swap_chain.image_count());
    for (auto& fence : m_waitFences)
    {
        vkCreateFence(m_device.m_vulkan_device, &fenceCreateInfo, nullptr, &fence);
    }
}

// End the command buffer and submit it to the queue
// Uses a fence to ensure command buffer has finished executing before deleting it
void Engine::flushGraphicsCommandBuffer_and_submit(VkCommandBuffer commandBuffer)
{
    assert(commandBuffer != VK_NULL_HANDLE);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo       = {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags             = 0;
    VkFence fence;
    (vkCreateFence(m_device.m_vulkan_device, &fenceCreateInfo, nullptr, &fence));

    // Submit to the queue
    (vkQueueSubmit(m_queue_manager.get_queue(m_graphics_queue), 1, &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    (vkWaitForFences(m_device.m_vulkan_device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

    vkDestroyFence(m_device.m_vulkan_device, fence, nullptr);
    vkFreeCommandBuffers(m_device.m_vulkan_device, m_queue_manager.get_command_pool(m_graphics_command_pool_data), 1, &commandBuffer);
}

// Get a new command buffer from the command pool
// If begin is true, the command buffer is also started so we can start adding commands
VkCommandBuffer Engine::getCommandBuffer(bool begin)
{
    VkCommandBuffer cmdBuffer;

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
    cmdBufAllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocateInfo.commandPool                 = m_queue_manager.get_command_pool(m_graphics_command_pool_data);
    cmdBufAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocateInfo.commandBufferCount          = 1;

    vkAllocateCommandBuffers(m_device.m_vulkan_device, &cmdBufAllocateInfo, &cmdBuffer);

    // If requested, also start the new command buffer
    if (begin)
    {
        VkCommandBufferBeginInfo cmdBufInfo = initializers::commandBufferBeginInfo();
        vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
    }

    return cmdBuffer;
}

void Engine::prepare_vertex_buffers()
{
    std::vector<Vertex>   all_vertices;
    std::vector<uint32_t> all_indexes;
    for (auto&& mesh : m_mesh_system.m_meshes)
    {
        for (auto&& vertex : mesh.m_vertex)
        {
            all_vertices.push_back(vertex);
        }
        for (auto&& idx : mesh.m_indices)
        {
            all_indexes.push_back(idx);
        }
    }
    m_vertex_buffer_graph = m_graph->m_resources.create_vertex_buffer(all_vertices);
    m_index_buffer_graph  = m_graph->m_resources.create_index_buffer(all_indexes);
}

// Vulkan loads its shaders from an immediate binary representation called SPIR-V
// Shaders are compiled offline from e.g. GLSL using the reference glslang compiler
// This function loads such a shader from a binary file and returns a shader module structure
VkShaderModule Engine::loadSPIRVShader(const std::string& filename)
{
    size_t shaderSize;
    char*  shaderCode = NULL;

    std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

    if (is.is_open())
    {
        is.seekg(0, std::ios::end);
        shaderSize = is.tellg();
        is.seekg(0, std::ios::beg);
        // Copy file contents into a buffer
        shaderCode = new char[shaderSize];
        is.read(shaderCode, shaderSize);
        is.close();
        assert(shaderSize > 0);
    }
    if (shaderCode)
    {
        // Create a new shader module that will be used for pipeline creation
        VkShaderModuleCreateInfo moduleCreateInfo{};
        moduleCreateInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.codeSize = shaderSize;
        moduleCreateInfo.pCode    = (uint32_t*)shaderCode;

        VkShaderModule shaderModule;
        (vkCreateShaderModule(m_device.m_vulkan_device, &moduleCreateInfo, NULL, &shaderModule));

        delete[] shaderCode;

        return shaderModule;
    }
    else
    {
        std::cerr << "Error: Could not open shader file \"" << filename << "\"" << std::endl;
        return VK_NULL_HANDLE;
    }

    // try to get the json data
}

class IncludeGeneric : public shaderc::CompileOptions::IncluderInterface
{ // Handles shaderc_include_resolver_fn callbacks.
    virtual shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth)
    {
        // this is a horrible hack...
        static std::vector<shaderc_include_result> shader_cache; // map... >.>

        auto&& found_itr = std::find_if(
            shader_cache.begin(), shader_cache.end(), [requested_source](const shaderc_include_result& item) { return std::strcmp(requested_source, item.source_name) == 0; });
        if (found_itr != shader_cache.end())
        {
            return &(*found_itr);
        }

        shader_cache.push_back(shaderc_include_result());
        shaderc_include_result& result = shader_cache.back();

        std::string shader_root_path = "../engine/data/shaders/glsl/";
        shader_root_path             = shader_root_path + requested_source;

        // path_tracker.

        std::ifstream is(shader_root_path, std::ios::in);
        result.source_name        = requested_source;
        result.source_name_length = strlen(result.source_name);
        // TODO: refactor this piece of shit...
        if (is.is_open())
        {
            is.seekg(0, std::ios::end);
            result.content_length = is.tellg();
            assert(result.content_length > 0);

            is.seekg(0, std::ios::beg);
            // Copy file contents into a buffer

            char* out_output_shader_code = new char[result.content_length];
            memset(out_output_shader_code, '\0', result.content_length + 1);
            is.read(out_output_shader_code, result.content_length);
            out_output_shader_code[result.content_length] = '\0';
            result.content                                = out_output_shader_code;

            is.close();
        }

        // result.source_name_length;
        // result.user_data;

        printf("Called");
        return &result;
    };

    // Handles shaderc_include_result_release_fn callbacks.
    virtual void ReleaseInclude(shaderc_include_result* data)
    {
        printf("Called");
    };
};

// this function is horrible...
// also, .pv, and .vert/.frag etc... should live in different folders
// binary shaders and actual glsl shaders
void Engine::get_SPIRVShadercode(const std::string& filename, std::vector<uint32_t>& out_output_shader_code, const VkShaderStageFlagBits stage)
{
    std::ifstream is(filename, std::ios::in);

    if (is.is_open())
    {
        is.seekg(0, std::ios::end);

        uint32_t out_shader_size = is.tellg();
        assert(out_shader_size > 0);

        is.seekg(0, std::ios::beg);
        // Copy file contents into a buffer
        char* shader_dode = new char[out_shader_size + 1];
        is.read(shader_dode, out_shader_size);
        shader_dode[out_shader_size] = '\0';
        // copy the content as uint32_t into the out_output_shader_code vector
        std::string code = shader_dode;
        while (!code.empty())
        {
            auto&& index = code.find(" ");
            if (index != -1)
            {
                unsigned int      x;
                std::stringstream hex_string;
                hex_string << std::dec << code.substr(0, index).c_str();
                uint32_t value;
                hex_string >> value;
                out_output_shader_code.push_back(value);
            }
            auto&& next_index = code.find(" ");
            if (next_index != -1)
            {
                code = code.substr(next_index + 1);
            }
            else
            {
                // code.clear();
            }
        }
        delete shader_dode;

        is.close();
    }
    else
    {
        // if it does not exist, we need to create it,
        // right now they are created modifying the name we received from filename
        // mesh.vert_bin.spv ->  mesh.vert
        const std::string new_shader_filename = filename.substr(0, filename.find("_"));
        std::ifstream     open_file(new_shader_filename, std::ios::in);

        if (open_file.is_open())
        {
            open_file.seekg(0, std::ios::end);

            uint32_t out_shader_size = open_file.tellg();
            assert(out_shader_size > 0);
            // TODO: refactor this piece of shit...
            open_file.seekg(0, std::ios::beg);
            // Copy file contents into a buffer
            char* shader_dode = new char[out_shader_size + 1];
            open_file.read(shader_dode, out_shader_size);
            shader_dode[out_shader_size] = '\0';
            open_file.close();

            shaderc::Compiler compiler; // probably this is a heavy operation?

            shaderc::CompileOptions compile_options;

            // IncluderInterface
            compile_options.SetIncluder(std::make_unique<IncludeGeneric>());

            compile_options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version::shaderc_env_version_vulkan_1_2);
            compile_options.SetTargetSpirv(shaderc_spirv_version::shaderc_spirv_version_1_5);
            compile_options.SetSourceLanguage(shaderc_source_language::shaderc_source_language_glsl);
            compile_options.SetWarningsAsErrors();
            compile_options.SetGenerateDebugInfo();
            // compile_options.SetSourceLanguage(shaderc_source_language::shaderc_source_language_glsl);

            shaderc_shader_kind shader_kind = get_shaderc_shader_kind(stage);
            compiler.IsValid();

            auto&& result = compiler.CompileGlslToSpv(shader_dode, shader_kind, new_shader_filename.c_str(), compile_options);
            // auto&& result2 = compiler.CompileGlslToSpvAssembly(shader_dode, shader_kind, new_shader_filename.c_str(), compile_options);

            printf("%s", result.GetErrorMessage().c_str());
            auto&& status       = result.GetCompilationStatus();
            auto&& num_warnings = result.GetNumWarnings();
            assert(status == 0);

            std::vector<uint32_t> remove_this_now_;
            printf("\n");

            std::ofstream output(filename, std::ios::out);
            for (auto&& value = result.begin(); value != result.end(); ++value)
            {
                out_output_shader_code.push_back(*value);
                output << *value << " ";
            }
            output.close();

            delete[] shader_dode;
        }
    }
}

shaderc_shader_kind Engine::get_shaderc_shader_kind(const VkShaderStageFlagBits stage)
{
    switch (stage)
    {
        case VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT: return shaderc_shader_kind::shaderc_vertex_shader;
        case VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT: return shaderc_shader_kind::shaderc_fragment_shader;
        default: assert(false);
    }
    return shaderc_shader_kind();
}

void Engine::render_loop()
{

    static bool first_frame = true;
    m_graph->m_tasks.clear();

    m_graph->m_pipeline_manager.m_current_descriptor_set_count.clear();
    {
        // TODO: This should not be in this level...
        for (uint32_t i = 0; i < m_graph->m_steps.size(); ++i)
        {
            m_graph->m_steps[i]->pre_execution_task(*m_graph);
        }
        for (uint32_t i = 0; i < m_graph->m_steps.size(); ++i)
        {
            m_graph->m_steps[i]->execution_task(*m_graph);
        }
    }

    m_graph->m_pipeline_manager.update();

    auto&& command_bufffer = m_queue_manager.get_command_buffer(m_graphics_command_buffers_data);
    auto&& r               = m_swap_chain.acquireNextImage(m_presentCompleteSemaphore, &m_current_buffers_idx);
    // Use a fence to wait until the command buffer has finished execution before using it again
    vkWaitForFences(m_device.m_vulkan_device, 1, &m_waitFences[m_current_buffers_idx], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device.m_vulkan_device, 1, &m_waitFences[m_current_buffers_idx]);

    command_bufffer.reset_command_buffer(m_current_buffers_idx, 0);
    command_bufffer.begin_command_buffer(m_current_buffers_idx, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    auto&& draw_command_buffer = command_bufffer.get_command_buffer(m_current_buffers_idx);

    {
        for (auto&& task : m_graph->m_tasks)
        {
            m_graph->m_pipeline_manager.process_memory_transitions(task->get_task_id(), draw_command_buffer);
            task->run(draw_command_buffer);
        }
    }

    {
        // assume that last task got the image we want to copy to the swapchain
        ColourAttachment* last_colour_attachment = nullptr;
        {
            // get last task colour attachment resource
            auto&& last_task_resource_collection = m_graph->m_tasks.back()->get_fragment_output();
            for (auto&& last_task_resource : last_task_resource_collection)
            {
                if (last_task_resource.second.m_resource->get_resource_type() == ResourceType::ColourAttachment)
                {
                    last_colour_attachment = &m_graph->m_resources.get_colour_attachment(*last_task_resource.second.m_resource);
                    break;
                }
            }
        }

        std::vector<VkImageMemoryBarrier2KHR> barriers;

        // Transition swapchain image from present to transfer dst layout
        VkImageMemoryBarrier2KHR memoryBarrier = {};
        memoryBarrier.sType                    = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        memoryBarrier.pNext;
        memoryBarrier.srcAccessMask    = VK_ACCESS_2_MEMORY_READ_BIT_KHR;
        memoryBarrier.dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        memoryBarrier.srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.image            = m_swap_chain.image(m_current_buffers_idx);
        memoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        memoryBarrier.srcQueueFamilyIndex;
        memoryBarrier.dstQueueFamilyIndex;

        barriers.push_back(memoryBarrier);

        // Transition last image from colour attachment to src transfer

        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        memoryBarrier.pNext;
        memoryBarrier.srcAccessMask    = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR;
        memoryBarrier.dstAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        memoryBarrier.srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.image            = last_colour_attachment->image();
        memoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        memoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        memoryBarrier.srcQueueFamilyIndex;
        memoryBarrier.dstQueueFamilyIndex;

        barriers.push_back(memoryBarrier);

        {
            VkDependencyInfoKHR dependencyInfo     = {};
            dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
            dependencyInfo.pImageMemoryBarriers    = barriers.data();
            dependencyInfo.imageMemoryBarrierCount = barriers.size();

            vkCmdPipelineBarrier2KHR(draw_command_buffer, &dependencyInfo);
            barriers.clear();
        }

        VkOffset3D blitSize;
        blitSize.x = last_colour_attachment->m_render_texture_info.extent.width;
        blitSize.y = last_colour_attachment->m_render_texture_info.extent.height;
        blitSize.z = 1;

        VkImageBlit imageBlitRegion{};
        imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.srcSubresource.layerCount = 1;
        imageBlitRegion.srcOffsets[1]             = blitSize;
        imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlitRegion.dstSubresource.layerCount = 1;
        imageBlitRegion.dstOffsets[1]             = blitSize;

        vkCmdBlitImage(
            draw_command_buffer, last_colour_attachment->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swap_chain.image(m_current_buffers_idx),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlitRegion, VK_FILTER_NEAREST);
        // VkImageCopy imageCopyRegion{};
        // imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // imageCopyRegion.srcSubresource.layerCount = 1;
        // imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // imageCopyRegion.dstSubresource.layerCount = 1;
        // imageCopyRegion.extent.width              = last_colour_attachment->m_render_texture_info.extent.width;
        // imageCopyRegion.extent.height             = last_colour_attachment->m_render_texture_info.extent.height;
        // imageCopyRegion.extent.depth              = 1;
        //
        // vkCmdCopyImage(
        //     draw_command_buffer, last_colour_attachment->image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_swap_chain.image(m_current_buffers_idx),
        //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

        // Transition swapchain image from transfer to present
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        memoryBarrier.pNext;
        memoryBarrier.srcAccessMask    = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
        memoryBarrier.dstAccessMask    = VK_ACCESS_2_MEMORY_READ_BIT_KHR;
        memoryBarrier.srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.image            = m_swap_chain.image(m_current_buffers_idx);
        memoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memoryBarrier.newLayout        = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        memoryBarrier.srcQueueFamilyIndex;
        memoryBarrier.dstQueueFamilyIndex;

        barriers.push_back(memoryBarrier);

        // Transition last image from src transfer to colour attachment

        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2_KHR;
        memoryBarrier.pNext;
        memoryBarrier.srcAccessMask    = VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
        memoryBarrier.dstAccessMask    = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR; // VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
        memoryBarrier.srcStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.dstStageMask     = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR;
        memoryBarrier.image            = last_colour_attachment->image();
        memoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        memoryBarrier.newLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        memoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        memoryBarrier.srcQueueFamilyIndex;
        memoryBarrier.dstQueueFamilyIndex;

        barriers.push_back(memoryBarrier);

        {
            VkDependencyInfoKHR dependencyInfo     = {};
            dependencyInfo.sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
            dependencyInfo.pImageMemoryBarriers    = barriers.data();
            dependencyInfo.imageMemoryBarrierCount = barriers.size();

            vkCmdPipelineBarrier2KHR(draw_command_buffer, &dependencyInfo);
            barriers.clear();
        }

        command_bufffer.end_command_buffer(m_current_buffers_idx);
    }

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo submitInfo      = {};
    submitInfo.sType             = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStageMask;                                                // Pointer to the list of pipeline stages that the semaphore waits will occur at
    submitInfo.pWaitSemaphores   = &m_presentCompleteSemaphore;                                   // Semaphore(s) to wait upon before the submitted
                                                                                                  // command buffer starts executing
    submitInfo.waitSemaphoreCount   = 1;                                                          // One wait semaphore
    submitInfo.pSignalSemaphores    = &m_renderCompleteSemaphore;                                 // Semaphore(s) to be signaled when command buffers have completed
    submitInfo.signalSemaphoreCount = 1;                                                          // One signal semaphore
    submitInfo.pCommandBuffers      = &command_bufffer.get_command_buffer(m_current_buffers_idx); // Command buffers(s) to execute in this batch (submission)
    submitInfo.commandBufferCount   = 1;                                                          // One command buffer

    // Submit to the graphics queue passing a wait fence
    vkQueueSubmit(m_queue_manager.get_queue(m_graphics_queue), 1, &submitInfo, m_waitFences[m_current_buffers_idx]);

    // Present the current buffer to the swap chain
    // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap
    // chain presentation This ensures that the image is not presented to the windowing system until all commands have
    // been submitted
    VkResult present = m_swap_chain.queuePresent(m_queue_manager.get_queue(m_graphics_queue), m_current_buffers_idx, m_renderCompleteSemaphore);
    if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR)))
    {
        assert(present == VK_SUCCESS);
    }
}

void Engine::update_frame()
{

    m_camera.update(delta_time);

    m_shadow_camera.m_location = glm::vec3(0.0, 150.0, 50.0);
    m_shadow_camera.rotation   = glm::vec3(-88.0f, 0.0f, 0.0f);

    m_shadow_camera.update_view_matrix();

    // render();

    render_loop();

    frameCounter++;

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    delta_time                                = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;
    lastUpdate                                = now;
    elapsed_time                              = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_running_time_stamp).count() / 1000.0f;
    lastFPS                                   = 1.0 / delta_time;

    // update overlay
    if (!m_device.m_settings.enable_imgui)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = ImVec2((float)m_swap_chain.width(), (float)m_swap_chain.height());
    io.DeltaTime   = delta_time;

    io.MousePos     = ImVec2(m_camera.mousePos.x, m_camera.mousePos.y);
    io.MouseDown[0] = m_camera.mouseButtons.left;
    io.MouseDown[1] = m_camera.mouseButtons.right;

    ImGui::NewFrame();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    // ImGui::SetNextWindowPos(ImVec2(10, 10));
    // ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_::ImGuiCond_FirstUseEver);
    ImGui::Begin("Info", nullptr);
    ImGui::Text("Entities %d", m_entity_system.m_registry.size());
    ImGui::TextUnformatted(m_device.m_device_properties.deviceName);
    ImGui::Text("Camera Pos : X[%0.2f] Y[%0.2f] Z[%0.2f]", m_camera.m_location.x, m_camera.m_location.y, m_camera.m_location.z);
    ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

    // ImGui::PushItemWidth(110.0f * m_ui.scale);
    // OnUpdateUIOverlay(&m_ui);
    ImGui::PopItemWidth();
    ImGui::End();

    /*
    //---- TEST AREA ---- //
    struct DATA_PIPELINE_DEFINITION
    {
        std::vector<VkDescriptorSetLayoutBinding> m_descriptor_set_layout;
    };
    static DATA_PIPELINE_DEFINITION m_data_pipeline_definition;

    ImGui::Begin("Pipeline step editor");

    if (ImGui::TreeNode("Pipeline config"))
    {
        ImGui::Text("Descriptor Set Layout");
        ImGui::SameLine();
        if (ImGui::Button("+##Descriptor Set Layout"))
        {
            m_data_pipeline_definition.m_descriptor_set_layout.push_back(VkDescriptorSetLayoutBinding());
        };
        for (auto&& descriptor_set_layout : m_data_pipeline_definition.m_descriptor_set_layout)
        {

            ImGui::InputInt("Binding", (int*)&descriptor_set_layout.binding);
            ImGui::InputInt("Binding", (int*)&descriptor_set_layout.descriptorCount);
        }


        ImGui::TreePop();
    }
    //ImGui::End();
    */

    ImGui::Begin("Transform test", nullptr);
    char t[120];
    if (ImGui::TreeNode("Transforms"))
    {
        auto& meshes_view = m_entity_system.m_registry.view<Mesh_renderer, Transform>();

        for (auto [entity, mesh, transform] : meshes_view.each())
        {
            glm::vec3 v;

            ImGui::Separator();
            ImGui::Text("Entity %d", entity);
            sprintf(t, "Pos: ##%d", entity);
            v = transform.pos();
            ;
            if (ImGui::DragFloat3(t, &v.x))
            {
                transform.set_pos(v);
            }
            v = transform.ori();
            ;
            sprintf(t, "Ori: ##%d", entity);
            if (ImGui::DragFloat3(t, &v.x))
            {
                transform.set_ori(v);
            };
            v = transform.sca();
            ;
            sprintf(t, "Sca: ##%d", entity);
            if (ImGui::DragFloat3(t, &v.x))
            {
                transform.set_sca(v);
            };
        }

        ImGui::TreePop();
    }

    ImGui::End();

    // ImGui::ShowDemoWindow();

    ImGui::PopStyleVar();
    ImGui::Render();

    // m_ui.update();
}

void Engine::render()
{
    // Get next image in the swap chain (back/front buffer)
    auto&& r = m_swap_chain.acquireNextImage(m_presentCompleteSemaphore, &m_current_buffers_idx);

    // Use a fence to wait until the command buffer has finished execution before using it again
    vkWaitForFences(m_device.m_vulkan_device, 1, &m_waitFences[m_current_buffers_idx], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device.m_vulkan_device, 1, &m_waitFences[m_current_buffers_idx]);

    /*Continous rebuild*/
    auto&& command_bufffer = m_queue_manager.get_command_buffer(m_graphics_command_buffers_data);
    command_bufffer.reset_command_buffer(m_current_buffers_idx, 0);

    /*
    {
        //auto&& last_pass = m_graph->m_flatten_passes.back();
        //auto&& last_pass_description = m_graph->m_node_descriptions[last_pass->get_idx()];

        auto&& clear_values = get_clear_values(glm::vec4(0.0f, 0.0f, 0.2f, 1.0f), 1.0f, 0);

        VkRenderPassBeginInfo renderPassBeginInfo =
            get_renderpass_begin_info(m_node_description.get_renderpass(), m_swap_chain.width(), m_swap_chain.height(),
    clear_values, m_swapchain_framebuffers.get_framebuffer(m_current_buffers_idx));
            //get_renderpass_begin_info(last_pass_description->get_renderpass(), m_swap_chain.width(),
    m_swap_chain.height(), clear_values, m_swapchain_framebuffers.get_framebuffer(m_current_buffers_idx));


        auto&& command_bufffer = m_queue_manager.get_command_buffer(m_graphics_command_buffers_data);
        command_bufffer.begin_command_buffer(m_current_buffers_idx, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        auto&& draw_command_buffer = command_bufffer.get_command_buffer(m_current_buffers_idx);

        vkCmdBeginRenderPass(draw_command_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    //--------------------------------
    //--------------------------------
    //--------------------------------

    // Update dynamic viewport state
        VkViewport viewport = {};
        viewport.height = (float)m_swap_chain.height();
        viewport.width = (float)m_swap_chain.width();
        viewport.minDepth = (float) 0.0f;
        viewport.maxDepth = (float) 1.0f;
        vkCmdSetViewport(draw_command_buffer, 0, 1, &viewport);

        // Update dynamic scissor state
        VkRect2D scissor = {};
        scissor.extent.width = m_swap_chain.width();
        scissor.extent.height = m_swap_chain.height();
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        vkCmdSetScissor(draw_command_buffer, 0, 1, &scissor);

        // Bind the rendering pipeline
        // The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states
    specified at pipeline creation time vkCmdBindPipeline(draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    m_node_description.get_pipeline());

    //--------------------------------
    //--------------------------------
    //--------------------------------
        draw_scene(draw_command_buffer);

    //--------------------------------
    //--------------------------------
    //--------------------------------

        vkCmdEndRenderPass(draw_command_buffer);

        command_bufffer.end_command_buffer(m_current_buffers_idx);

    }

    */
    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo submitInfo         = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask    = &waitStageMask;                                             // Pointer to the list of pipeline stages that the semaphore waits will occur at
    submitInfo.pWaitSemaphores      = &m_presentCompleteSemaphore;                                // Semaphore(s) to wait upon before the submitted command buffer starts executing
    submitInfo.waitSemaphoreCount   = 1;                                                          // One wait semaphore
    submitInfo.pSignalSemaphores    = &m_renderCompleteSemaphore;                                 // Semaphore(s) to be signaled when command buffers have completed
    submitInfo.signalSemaphoreCount = 1;                                                          // One signal semaphore
    submitInfo.pCommandBuffers      = &command_bufffer.get_command_buffer(m_current_buffers_idx); // Command buffers(s) to execute in this batch (submission)
    submitInfo.commandBufferCount   = 1;                                                          // One command buffer

    // Submit to the graphics queue passing a wait fence
    vkQueueSubmit(m_queue_manager.get_queue(m_graphics_queue), 1, &submitInfo, m_waitFences[m_current_buffers_idx]);

    // Present the current buffer to the swap chain
    // Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap
    // chain presentation This ensures that the image is not presented to the windowing system until all commands have
    // been submitted
    VkResult present = m_swap_chain.queuePresent(m_queue_manager.get_queue(m_graphics_queue), m_current_buffers_idx, m_renderCompleteSemaphore);
    if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR)))
    {
        assert(present == VK_SUCCESS);
    }
}

//--------------------------------------------------------------//
//--------------------------------------------------------------//
