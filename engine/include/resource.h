#pragma once

#include <id.h>
#include <memory>

enum class ResourceType
{
    // Task,
    TaskDescription,
    ColourAttachment,
    DepthAttachment,
    UBO,
    SSBO,
    VertexBuffer,
    IndexBuffer,
    Texture,
    StorageTexture,
    Sampler,
    Count,
};

class Resource : public ID
{
    friend class RenderGraphBuilder;

  public:
    // void				set_name(const std::string& name) { m_name = name; }
    // const std::string&	get_name() const { return m_name; }

    const ResourceType get_resource_type() const
    {
        return m_resource_type;
    }
    void set_resource_type(ResourceType resource_type)
    {
        m_resource_type = resource_type;
    }

    Resource(){}; // TODO: >.>

    // move to private...
    std::shared_ptr<Resource> copy_with_alias(const uint32_t new_alias)
    {
        std::shared_ptr<Resource> to_return = std::make_shared<Resource>();
        to_return->m_alias_id               = new_alias;
        to_return->m_resource_id            = m_resource_id;
        to_return->m_resource_type          = m_resource_type;

        return to_return;
    };

    const bool point_to_same_resource(const Resource& other)
    {
        return other.m_resource_id == m_resource_id && other.m_resource_type == m_resource_type;
    };

    uint32_t m_resource_id = -1;

    // -1 means the actual resource is hold in here
    // when a resource is set to any task
    // a copy of this Resource is created, but the alias is set to the origin task of the resource (output of the task consuming this)
    uint32_t m_alias_id = -1;

    ResourceType m_resource_type;
};