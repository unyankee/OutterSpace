#include <engine.h>
#include <step.h>

// CCW
static uint32_t local_cube_indices[] = {
    // Top
    3, 2, 6, 6, 7, 3,

    // Bottom
    5, 1, 0, 0, 4, 5,

    // Left
    10, 8, 9, 9, 11, 10,

    // Right
    13, 12, 14, 14, 15, 13,

    // Front
    16, 17, 18, 18, 19, 16,

    // Back
    22, 21, 20, 20, 23, 22};

static Vertex local_cube_vertices[] = {
    /*0*/ {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},  /*0 */
    /*1*/ {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},   /*1 */
    /*2*/ {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},     /*2*/
    /*3*/ {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},    /*3 */
    /*4*/ {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}}, /*4 */
    /*5*/ {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}},  /*5 */
    /*6*/ {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},    /*6 */
    /*7*/ {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
    /*7 */                                              //
    /*0*/ {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},  /*8 */
    /*3*/ {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},   /*9 */
    /*4*/ {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}}, /*10*/
    /*7*/ {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}},
    /*11*/                                            //
    /*1*/ {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},  /*12*/
    /*2*/ {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},   /*13*/
    /*5*/ {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}, /*14*/
    /*6*/ {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
    /*15*/                                            //
    /*0*/ {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, /*16*/
    /*1*/ {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},  /*17*/
    /*2*/ {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},   /*18*/
    /*3*/ {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    /*19*/                                              //
    /*4*/ {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}}, /*20*/
    /*5*/ {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},  /*21*/
    /*6*/ {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},   /*22*/
    /*7*/ {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}},
    /*23*/ //
};

MainStep::MainStep()
{
    // subscribe to steps lists
    // TODO: remove singleton, create the user entry point, passing down the graph
    // the user will subscribe steps to
    // Engine::instance().m_graph->m_steps.push_back(this);
}

void MainStep::create_resources(Graph& graph)
{
    {
        // create single technique used by this step
        const ShaderConfig vertex1("main", "../engine/data/shaders/glsl/mesh/mesh.vert_bin.spv", VK_SHADER_STAGE_VERTEX_BIT);
        const ShaderConfig fragment1("main", "../engine/data/shaders/glsl/mesh/mesh.frag_bin.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<ShaderConfig> shader_config;
        shader_config.push_back(vertex1);
        shader_config.push_back(fragment1);
        m_task_description1 = graph.create_task_description(shader_config);
    }
    {
        // create vertex buffer used by this step
        struct VertexData
        {
            float position[3];
            float normal[3];
        };
        VertexDefinition cube_vertex_definition;
        cube_vertex_definition.m_input_descriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, position)});
        cube_vertex_definition.m_input_descriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexData, normal)});
        cube_vertex_definition.m_bindings.push_back({/*binding*/ 0, /*stride*/ 0, VkVertexInputRate::VK_VERTEX_INPUT_RATE_VERTEX});

        m_vertex_buffer = graph.m_resources.create_vertex_buffer(
            cube_vertex_definition, local_cube_vertices, sizeof(local_cube_vertices), sizeof(local_cube_vertices) / sizeof(local_cube_vertices[0]));

        m_index_buffer = graph.m_resources.create_index_buffer(local_cube_indices, sizeof(local_cube_indices), sizeof(local_cube_indices) / sizeof(local_cube_indices[0]));
    }
    {
        // Colour attachment 0 creation
        glm::uvec2 full_screen_resolution = graph.get_fullscreen_resolution();

        TextureInfo info      = {};
        info.imageType        = VK_IMAGE_TYPE_2D;
        info.format           = VK_FORMAT_R8G8B8A8_UNORM;
        info.extent           = {full_screen_resolution.x, full_screen_resolution.y, 1};
        info.samples          = VK_SAMPLE_COUNT_1_BIT;
        info.usage            = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.m_current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.attachmentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        info.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        info.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        info.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;

        m_colour_attachment0 = graph.m_resources.create_colour_attachment(info);
    }
    {
        // Colour attachment 0 creation
        glm::uvec2 full_screen_resolution = graph.get_fullscreen_resolution();

        TextureInfo info      = {};
        info.imageType        = VK_IMAGE_TYPE_2D;
        info.format           = graph.get_depth_format();
        info.extent           = {full_screen_resolution.x, full_screen_resolution.y, 1};
        info.samples          = VK_SAMPLE_COUNT_1_BIT;
        info.usage            = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        info.m_current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.attachmentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        info.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        info.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        info.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;

        m_depth_attachment = graph.m_resources.create_depth_attachment(info);
    }
    {
        // prepare m_camera_info
        BufferInfo info;
        info.buffer_usage      = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        info.size              = sizeof(m_scene_data_cpu);
        m_camera_info          = graph.m_resources.create_ubo(info);
    }
    {
        // create world matrixes instances
        std::vector<glm::mat4> all_model_matrixes;
        all_model_matrixes.push_back(glm::identity<glm::mat4>());

        BufferInfo info;
        info.buffer_usage      = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        info.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        info.size              = sizeof(glm::mat4) * all_model_matrixes.size();

        m_instances_data = graph.m_resources.create_ssbo(info);
    }
}

void MainStep::pre_execution_task(Graph& graph)
{
    {
        // update camera buffer
        auto&& camera                     = graph.m_engine.m_camera;
        m_scene_data_cpu.camera_position  = glm::vec4(camera.m_location.x, camera.m_location.y, camera.m_location.z, graph.m_engine.elapsed_time);
        m_scene_data_cpu.projectionMatrix = camera.perspective;
        m_scene_data_cpu.viewMatrix       = camera.view;

        graph.m_resources.update_UBO(m_camera_info, &m_scene_data_cpu, sizeof(m_scene_data_cpu));
    }
    {
        std::vector<glm::mat4> all_model_matrixes;
        all_model_matrixes.push_back(glm::identity<glm::mat4>());
        graph.m_resources.update_SSBO(m_instances_data, all_model_matrixes.data(), sizeof(glm::mat4) * all_model_matrixes.size());
    }
}

void MainStep::execution_task(Graph& graph)
{
    static Parameter colour_attachment0("outFragColor");
    static Parameter camera_data("scene_data");
    static Parameter dynamic_world_matrixes("instance_data");

    std::shared_ptr<Task> task1 = graph.create_task(m_task_description1);
    task1->set_resource(colour_attachment0, m_colour_attachment0);
    task1->set_resource(depth_attachment_id, m_depth_attachment);
    task1->set_resource(camera_data, m_camera_info);
    task1->set_resource(dynamic_world_matrixes, m_instances_data);
    task1->set_resource(vertex_buffer_id, m_vertex_buffer);
    task1->set_resource(index_buffer_id, m_index_buffer);
    task1->set_instance_count(m_instances);
    task1->set_vertex_offset(0);
}
//

UIStep::UIStep()
{
    // Init ImGui
    ImGui::CreateContext();
    // Color scheme
    ImGuiStyle& style                       = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg]          = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
    style.Colors[ImGuiCol_MenuBarBg]        = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_Header]           = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_FrameBg]          = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_CheckMark]        = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_SliderGrab]       = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
    style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
    style.Colors[ImGuiCol_Button]           = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
    style.Colors[ImGuiCol_ButtonActive]     = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
    // Dimensions
    ImGuiIO& io        = ImGui::GetIO();
    io.FontGlobalScale = scale;

    build_imgui_resources();
    // preparePipeline(m_engine->m_node_description.get_renderpass());
}

void UIStep::build_imgui_resources()
{
    /* ImGuiIO& io = ImGui::GetIO();
    // Create font texture
    unsigned char* fontData;
    int            texWidth, texHeight;

    const std::string filename = "../engine/data/fonts/Roboto-Medium.ttf";
    io.Fonts->AddFontFromFileTTF(filename.c_str(), 16.0f);

    io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
    VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

    // Create target image for copy
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width  = texWidth;
    imageInfo.extent.height = texHeight;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vkCreateImage(m_engine->m_device.m_vulkan_device, &imageInfo, nullptr, &fontImage);
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(m_engine->m_device.m_vulkan_device, fontImage, &memReqs);
    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize       = memReqs.size;
    memAllocInfo.memoryTypeIndex      = m_engine->getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(m_engine->m_device.m_vulkan_device, &memAllocInfo, nullptr, &fontMemory);
    vkBindImageMemory(m_engine->m_device.m_vulkan_device, fontImage, fontMemory, 0);
    //

    // Image view
    VkImageViewCreateInfo viewInfo       = {};
    viewInfo.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image                       = fontImage;
    viewInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                      = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(m_engine->m_device.m_vulkan_device, &viewInfo, nullptr, &fontView);

    // STAGING BUFFER
    StagingBuffer stagingbuffer;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.size               = uploadSize;
    (vkCreateBuffer(m_engine->m_device.m_vulkan_device, &bufferCreateInfo, nullptr, &stagingbuffer.buffer));

    // Create the memory backing up the buffer handle
    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    vkGetBufferMemoryRequirements(m_engine->m_device.m_vulkan_device, stagingbuffer.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    // Find a memory type index that fits the properties of the buffer
    memAlloc.memoryTypeIndex = m_engine->getMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // TODO: REMOVE !!!
    // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (VK_BUFFER_USAGE_TRANSFER_SRC_BIT & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        memAlloc.pNext       = &allocFlagsInfo;
    }
    vkAllocateMemory(m_engine->m_device.m_vulkan_device, &memAlloc, nullptr, &stagingbuffer.memory);

    // buffer->alignment = memReqs.alignment;
    // buffer->size = uploadSize;
    // buffer->usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    // buffer->memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Initialize a default descriptor that covers the whole buffer size
    // buffer->setupDescriptor();

    void* data;
    vkMapMemory(m_engine->m_device.m_vulkan_device, stagingbuffer.memory, 0, memAlloc.allocationSize, 0, &data);
    memcpy(data, fontData, uploadSize);
    vkUnmapMemory(m_engine->m_device.m_vulkan_device, stagingbuffer.memory);

    vkBindBufferMemory(m_engine->m_device.m_vulkan_device, stagingbuffer.buffer, stagingbuffer.memory, 0);

    // THIS NEEDS TO BE REVISIT, URGENTLY !!

    VkCommandBuffer copyCmd = m_engine->getCommandBuffer(true);

    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = 1;
        subresourceRange.layerCount              = 1;

        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.image            = fontImage;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask    = 0;
        imageMemoryBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    // Copy
    VkBufferImageCopy bufferCopyRegion           = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width           = texWidth;
    bufferCopyRegion.imageExtent.height          = texHeight;
    bufferCopyRegion.imageExtent.depth           = 1;

    vkCmdCopyBufferToImage(copyCmd, stagingbuffer.buffer, fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

    {
        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = 1;
        subresourceRange.layerCount              = 1;

        // Create an image barrier object
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageMemoryBarrier.image            = fontImage;
        imageMemoryBarrier.subresourceRange = subresourceRange;
        imageMemoryBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask    = VK_ACCESS_SHADER_READ_BIT;
        // imageMemoryBarrier.srcAccessMask = 0;
        // imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // Put barrier inside setup command buffer
        vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }

    m_engine->flushGraphicsCommandBuffer_and_submit(copyCmd);

    vkDestroyBuffer(m_engine->m_device.m_vulkan_device, stagingbuffer.buffer, nullptr);
    vkFreeMemory(m_engine->m_device.m_vulkan_device, stagingbuffer.memory, nullptr);
    //////

    // Font texture Sampler
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.maxAnisotropy       = 1.0f;
    samplerInfo.magFilter           = VK_FILTER_LINEAR;
    samplerInfo.minFilter           = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    (vkCreateSampler(m_engine->m_device.m_vulkan_device, &samplerInfo, nullptr, &sampler));

    VkDescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorPoolSize.descriptorCount = 1;
    // Descriptor pool
    std::vector<VkDescriptorPoolSize> poolSizes = {descriptorPoolSize};

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes    = poolSizes.data();
    descriptorPoolInfo.maxSets       = 2;

    (vkCreateDescriptorPool(m_engine->m_device.m_vulkan_device, &descriptorPoolInfo, nullptr, &descriptorPool));

    // Descriptor set layout
    VkDescriptorSetLayoutBinding setLayoutBinding{};
    setLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    setLayoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    setLayoutBinding.binding         = 0;
    setLayoutBinding.descriptorCount = 1;

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {setLayoutBinding};

    VkDescriptorSetLayoutCreateInfo descriptorLayout{};
    descriptorLayout.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayout.pBindings    = setLayoutBindings.data();
    descriptorLayout.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

    (vkCreateDescriptorSetLayout(m_engine->m_device.m_vulkan_device, &descriptorLayout, nullptr, &descriptorSetLayout));

    // Descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptorPool;
    allocInfo.pSetLayouts        = &descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;
    (vkAllocateDescriptorSets(m_engine->m_device.m_vulkan_device, &allocInfo, &descriptorSet));

    VkDescriptorImageInfo fontDescriptor{};
    fontDescriptor.sampler     = sampler;
    fontDescriptor.imageView   = fontView;
    fontDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet          = descriptorSet;
    writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorSet.dstBinding      = 0;
    writeDescriptorSet.pImageInfo      = &fontDescriptor;
    writeDescriptorSet.descriptorCount = 1;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {writeDescriptorSet};
    vkUpdateDescriptorSets(m_engine->m_device.m_vulkan_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    */
}

void UIStep::create_resources(Graph& graph)
{
    { // create single technique used by this step
        const ShaderConfig vertex1("main", "../engine/data/shaders/glsl/imgui/vertex.vert_bin.spv", VK_SHADER_STAGE_VERTEX_BIT);
        const ShaderConfig fragment1("main", "../engine/data/shaders/glsl/imgui/fragment.frag_bin.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        std::vector<ShaderConfig> shader_config;
        shader_config.push_back(vertex1);
        shader_config.push_back(fragment1);
        m_task_description = graph.create_task_description(shader_config);
    }
}

void UIStep::pre_execution_task(Graph& graph)
{
}

void UIStep::execution_task(Graph& graph)
{
}
