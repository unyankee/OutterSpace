#pragma once
#include <engine.h>

#include "imgui.h"

// Step executed inside the render loop
class Step
{
  public:
    virtual void create_resources(class Graph& graph)   = 0;
    virtual void execution_task(class Graph& graph)     = 0;
    virtual void pre_execution_task(class Graph& graph) = 0;
};

class MainStep : public Step
{
  public:
    // investigate another way of doingthis, ideally I want to remove the singleton this is
    // making use of
    MainStep();
    void create_resources(class Graph& graph) final override;
    void pre_execution_task(class Graph& graph) final override;
    void execution_task(class Graph& graph) final override;

    std::shared_ptr<class Resource> m_task_description1;

    std::shared_ptr<class Resource> m_camera_info;
    std::shared_ptr<class Resource> m_instances_data;

    std::shared_ptr<class Resource> m_vertex_buffer;
    std::shared_ptr<class Resource> m_index_buffer;

    std::shared_ptr<class Resource> m_depth_attachment;
    std::shared_ptr<class Resource> m_colour_attachment0;

    const uint32_t m_instances = 1;

    struct SCENE_DATA
    {
        glm::mat4 projectionMatrix;
        glm::mat4 viewMatrix;
        glm::vec4 camera_position;
    } m_scene_data_cpu;
};

class UIStep : public Step
{
  public:
    UIStep();
    void build_imgui_resources();

    void create_resources(class Graph& graph) final override;
    void pre_execution_task(class Graph& graph) final override;
    void execution_task(class Graph& graph) final override;

    std::shared_ptr<class Resource> m_task_description;

    bool  visible = true;
    bool  updated = false;
    float scale   = 1.0f;
};
