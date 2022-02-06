#pragma once

#include <VulkanSDK/1.2.198.1/Include/vulkan/vulkan.h>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <camera.h>
#include <entt/single_include/entt/entt.hpp>
#include <ui.h>

#include <tuple>

#include <optional>

#include <shaderc.hpp>

// Engine includes
#include <buffers.h>
#include <buffers_description.h>
#include <graph.h>
#include <id.h>
#include <parameter.h>
#include <pipeline_manager.h>
#include <queue_manager.h>
#include <resource.h>
#include <shader_reflection.h>
#include <step.h>
#include <swapchain.h>
#include <task.h>
#include <task_description.h>
#include <vulkan_device.h>

// TODO: IMPORTANT !!!
// query maxColorAttachments, usually 8, but query it...

// gtx 1080 right now...

constexpr uint32_t g_max_color_attachments = 8;
// constexpr uint32_t g_max_vertex_input_bindings			= 32;
constexpr uint32_t g_max_vertex_input_bindings = 6;
// constexpr uint32_t g_max_descriptor_set_uniform_buffers = 180;	// needs to be smaller than this, this is huge
constexpr uint32_t g_max_descriptor_set_uniform_buffers = 8; // needs to be smaller than this, this is huge
// constexpr uint32_t g_max_descriptor_set_storage_buffers = 1048576;	// needs to be smaller than this, this is huge
constexpr uint32_t g_max_descriptor_set_storage_buffers = 12; // needs to be smaller than this, this is huge

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

enum class SYSTEM_ID : uint32_t
{
    NON_SET = 0,
    RENDER,
    MESH,
};

// CCW
static uint32_t cube_indices[] = {
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

static Vertex cube_vertices[] = {
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

static Vertex triangle_vertexBuffer[] = {{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}, {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}, {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
// CCW
static uint32_t triangle_indexBuffer[] = {0, 1, 2};

class System
{
  public:
    System()
    {
        SYSTEM_ID = SYSTEM_ID::NON_SET;
    };
    SYSTEM_ID    SYSTEM_ID;
    virtual void update() = 0;

  private:
    // copies / move / assignments are not valid operations with this class
    // explcitly says so.
    System(const System& other) = delete;
    System(System&& other)      = delete;
    System& operator=(System const&) = delete;
};

// todo:
// TO PARENT AN ENTITY, TRY TO STORE THE INT, ID OF THE PARENT ENTITYT
// AND ADD THAT TO EACH COMPONENT WHEN CRETED, SO IT CAN BE ACCESSED AT ANY POINT

class Entity
{
    friend class EntitySystem;
    friend class Engine;

  public:
    Entity() // this might be completely wrong...
        {

        };

    template <class Component> Component& add_component()
    {
        // assert m_entity != null
        return Engine::instance().m_entity_system.add_component<Component>(m_entity);
    }

    template <class Component> Component& add_component(Component& component)
    {
        // assert m_entity != null
        return Engine::instance().m_entity_system.add_component<Component>(m_entity, component);
    }

    template <class Component> Component& get_component()
    {
        // assert m_entity != null
        return Engine::instance().m_entity_system.get_component<Component>(m_entity);
    }

  private:
    entt::entity m_entity = entt::null;
};

class EntitySystem
{
    friend class Entity;
    friend class Engine;
    // supposed to use this, in order to create entities?

  private:
    // very simple entity creation mechanism, needs a lot of work, testing by now...
    Entity create_new_entity()
    {
        Entity new_entity;
        new_entity.m_entity = m_registry.create();
        return new_entity;
    }

    template <class Component> Component& add_component(entt::registry::entity_type& entity)
    {
        return m_registry.emplace<Component>(entity);
    }
    template <class Component> Component& add_component(entt::registry::entity_type& entity, Component& component)
    {
        return m_registry.emplace<Component>(entity, component);
    }

    template <class Component> Component& get_component(entt::registry::entity_type& entity)
    {
        return m_registry.get<Component>(entity);
    }
    entt::registry m_registry;
};

struct Transform
{
    void set_pos(glm::vec3& pos)
    {
        set_dirty();
        m_location = pos;
    };
    void set_ori(glm::vec3& ori)
    {
        set_dirty();
        m_rotation = ori;
    };
    void set_sca(glm::vec3& sca)
    {
        set_dirty();
        m_scale = sca;
    };

    const glm::vec3& pos() const
    {
        return m_location;
    };
    const glm::vec3& ori() const
    {
        return m_rotation;
    };
    const glm::vec3& sca() const
    {
        return m_scale;
    };

    const glm::mat4& world_matrix() const
    {
        return m_world_matrix;
    };

    void update_world_matrix()
    {
        set_dirty(); // dafuck?
        m_world_matrix = glm::translate(glm::mat4(1.0f), m_location);
        m_world_matrix = glm::rotate(m_world_matrix, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        m_world_matrix = glm::rotate(m_world_matrix, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        m_world_matrix = glm::rotate(m_world_matrix, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        m_world_matrix = glm::scale(m_world_matrix, m_scale);
        clear_dirty();
    };

    const bool dirty() const
    {
        return m_dirty;
    }

  private:
    void set_dirty()
    {
        m_dirty = true;
    };
    void clear_dirty()
    {
        m_dirty = false;
    };

    glm::vec3 m_location;
    glm::vec3 m_scale;
    glm::vec3 m_rotation;

    // think about this...>.>
    glm::mat4 m_world_matrix;
    bool      m_dirty = true;
};

struct Mesh
{
    Mesh()
    {
    }

  public:
    std::vector<Vertex>   m_vertex;
    std::vector<uint32_t> m_indices;
    std::vector<float>    m_normals;
    std::vector<float>    m_uvs; // vector2
    uint32_t              m_id                  = -1;
    uint32_t              m_vertex_offset_bytes = 0;
    uint32_t              m_index_offset_bytes  = 0;
    uint32_t              m_vertex_offset       = 0;
    uint32_t              m_index_offset        = 0;
};

struct Mesh_renderer
{
    void set_mesh(const Mesh& mesh)
    {
        m_mesh_id = mesh.m_id;
    }
    const uint32_t& get_mesh_id() const
    {
        return m_mesh_id;
    }

  private:
    uint32_t m_mesh_id;
};

class MeshSystem : public System
{
  public:
    void register_new_mesh(Mesh& mesh)
    {
        if (!m_meshes.empty())
        {
            const Mesh& previous_mesh  = m_meshes.back();
            mesh.m_index_offset        = previous_mesh.m_index_offset + previous_mesh.m_indices.size();
            mesh.m_vertex_offset       = previous_mesh.m_vertex_offset + previous_mesh.m_vertex.size();
            mesh.m_index_offset_bytes  = previous_mesh.m_index_offset_bytes + (previous_mesh.m_indices.size() * sizeof(uint32_t));
            mesh.m_vertex_offset_bytes = previous_mesh.m_vertex_offset_bytes + (previous_mesh.m_vertex.size() * sizeof(uint32_t));
        }

        mesh.m_id = m_meshes.size();
        m_meshes.push_back(mesh);
    };
    MeshSystem()
    {
        SYSTEM_ID = SYSTEM_ID::MESH;
    };
    void update() override{};

    std::vector<Mesh> m_meshes;
};

class EntitySystem;
class MeshSystem;
class Entity;

class Engine
{
    friend class Swapchain;

  public:
    bool enable_validation_layer = true;

    Engine();
    static Engine& instance()
    {
        static Engine* engine = new Engine();
        return *engine;
    }

    ui m_ui;

    // Functions
  public:
    void init();
    void update();

    // "common" startup application
    bool init_device();
    void setup_window();
    void prepare();
    void create_synchronization_primitives();
    void prepare_synchronization_primitives();
    void prepare_vertex_buffers();
    // revisit this, looks wrong... >.>
    VkCommandBuffer getCommandBuffer(bool begin);
    void            flushGraphicsCommandBuffer_and_submit(VkCommandBuffer commandBuffer);

    VkShaderModule loadSPIRVShader(const std::string& filename);

    void get_SPIRVShadercode(const std::string& filename, std::vector<uint32_t>& out_output_shader_code, const VkShaderStageFlagBits stage);

    PFN_vkCmdBeginRenderingKHR   vkCmdBeginRenderingKHR;
    PFN_vkCmdEndRenderingKHR     vkCmdEndRenderingKHR;
    PFN_vkCmdPipelineBarrier2KHR vkCmdPipelineBarrier2KHR;

  private:
    shaderc_shader_kind get_shaderc_shader_kind(const VkShaderStageFlagBits stage);

  public:
    void  render_loop();
    void  update_frame();
    float fpsTimer = 0.1f;
    void  render();

    //-------------------------//
    //-------------------------//
    Entity create_entity()
    {
        return m_entity_system.create_new_entity();
    };
    EntitySystem        m_entity_system;
    MeshSystem          m_mesh_system;
    std::vector<Entity> m_test_entity;
    //-------------------------//
    //-------------------------//

    // Frame counter to display fps
    uint32_t                              frameCounter = 0;
    uint32_t                              lastFPS      = 0;
    std::chrono::steady_clock::time_point lastUpdate;
    std::chrono::steady_clock::time_point start_running_time_stamp;
    /** @brief Last frame time measured using a high performance timer (if available) */
    float delta_time   = 0.0f;
    float elapsed_time = 0.0;

    // Active frame buffer index
    uint32_t m_current_buffers_idx = 0;

    HWND      m_HWND_window;
    HINSTANCE HINSTANCE_windowInstance;

    Camera m_camera;
    Camera m_shadow_camera;

    bool resizing = false;

    unsigned int destWidth;
    unsigned int destHeight;

    void handle_messages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void window_resize();

    // private:

    VulkanDevice m_device;

    Swapchain m_swap_chain;

    // Fences
    // Used to check the completion of queue operations (e.g. command buffer execution)
    std::vector<VkFence> m_waitFences;

    std::shared_ptr<class Resource> m_texture_out_test;
    std::shared_ptr<class Resource> m_rendergraph_depth_attachment;
    std::shared_ptr<class Resource> m_directional_light_depth_attachment;

    // std::shared_ptr<class RenderGraphResource> m_entities_transform;
    std::shared_ptr<class Resource> m_global_scene_data;
    std::shared_ptr<class Resource> m_global_ssbo;

    std::shared_ptr<class Resource> m_vertex_buffer_graph;
    std::shared_ptr<class Resource> m_index_buffer_graph;

  public:
    // Attributes

    void* deviceCreatepNextChain = nullptr;

    const uint32_t getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const;

    /*
    --------------------------------
    --------------------------------
    --------  DEMO AREA  -----------
    --------------------------------
    --------------------------------
    */

    bool prepared = false;
    // void destroyCommandBuffers();

    // Synchronization primitives
    // Synchronization is an important concept of Vulkan that OpenGL mostly hid away. Getting this right is crucial to using Vulkan.

    // Semaphores
    // Used to coordinate operations within the graphics queue and ensure correct command ordering
    VkSemaphore m_presentCompleteSemaphore;
    VkSemaphore m_renderCompleteSemaphore;

    // Descriptor set pool
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

    // For simplicity we use the same uniform block layout as in the shader:
    //
    //	layout(set = 0, binding = 0) uniform UBO
    //	{
    //		mat4 projectionMatrix;
    //		mat4 modelMatrix;
    //		mat4 viewMatrix;
    //	} ubo;
    //
    // This way we can just memcopy the ubo data to the ubo
    // Note: You should use data types that align with the GPU in order to avoid manual padding (vec4, mat4)
    struct SCENE_DATA
    {
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
        glm::vec4 camera_position;
    } m_scene_data_cpu;

    std::shared_ptr<Buffer> m_scene_data_buffer;
    std::vector<Buffer>     m_ssbo;

    // The descriptor set stores the resources bound to the binding points in a shader
    // It connects the binding points of the different shaders with the buffers and images used for those bindings
    // VkDescriptorSet m_descriptorSet;
    std::vector<VkDescriptorSet> m_descriptor_sets;

    // The descriptor set layout describes the shader binding layout (without actually referencing descriptor)
    // Like the pipeline layout it's pretty much a blueprint and can be used with different descriptor sets as long as their layout matches
    // VkDescriptorSetLayout m_descriptorSetLayout;
    // DescriptorSetLayout m_descriptor_set_layout;

    QueueManager m_queue_manager;

    QueueData         m_graphics_queue;
    CommandPoolData   m_graphics_command_pool_data;
    CommandBufferData m_graphics_command_buffers_data;

    QueueData         m_transfer_queue;
    CommandPoolData   m_transfer_command_pool_data;
    CommandBufferData m_transfer_command_buffers_data;

    std::unique_ptr<class Graph> m_graph;

    // investigate a better way of handling this
    MainStep m_main_step;
};

/*
// testing some different approach...
// there is a collection of 'resources' that does not make any sense if we name them
//like any other resource, we can have a name for ssbo, ubo, rendertargets even
// since in the shader we are explicitly naming them
// but this is not applied to the depth, since there is no need to name it in the shader
// so... this will be a value in the node itself
// using some pre generated name

// TODO: investigate this and improve it / remove it
*/
static Parameter depth_attachment_id("depth_attachment");
static Parameter vertex_buffer_id("vertex_buffer_id");      // need to check maxVertexInputBindings,  right now support only 1
static Parameter index_buffer_id("index_buffer_id");        // some as above on here...
static Parameter instances_to_draw_id("instances_to_draw"); // some as above on here...

// TODO: link each Task to a pipeline description
// and each pass, will make use of the in/out resources that it has to fill the missing
// actual memory data
// mmmm... wrong?
// what about, each call, set up a bunch of code,
// that will be used to know which pipeline to build, and group calls into them?

static VkDescriptorType map_resource_type_to_vkdescriptortype(const ResourceType resource)
{
    switch (resource)
    {
        // case ResourceType::BackBuffer: return ;
        case ResourceType::SSBO: return VkDescriptorType::VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ResourceType::UBO: return VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        default: assert(false);
    }
}
