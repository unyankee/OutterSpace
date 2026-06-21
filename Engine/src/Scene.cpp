#include "Scene.h"
#include "Common/Common.h"

namespace ToyEngine
{
    
    Actor Scene::createActor()
    {
        entt::entity entity = m_registry.create();
        // Every Actor will be rendered, so we assign a transform to it

        const uint32_t transformIndex = transformSystem.alloc();
        m_registry.emplace<TransformIndex>(entity, transformIndex);
        
        return Actor(entity, this);
    }

    void Scene::update()
    {
        // Right now builds a list of what needs to be rendered
        // Sorted by Mesh id
        auto view = getRegistry().view<Mesh*, TransformIndex>();
        for (const auto& [entity, mesh, transformIndex]  : view.each())
        {
            
        }
        
    }

    Transform& TransformManager::getTransform(Actor& actor)
    {
        const TransformIndex& transIdx = actor.getComponent<TransformIndex>();
        return TransformsData[transIdx.index];
    }

    uint32_t TransformManager::alloc()
    {
        if (!freeList.empty())
        {
            uint32_t idx = freeList.back();
            freeList.pop_back();
            return idx;
        }
        TransformsData.push_back({});
        return (uint32_t)TransformsData.size() - 1;
    }

    void TransformManager::free(uint32_t idx)
    {
        freeList.push_back(idx);
    }

    void TransformManager::update()
    {
        for (Transform& transform : TransformsData) {
            glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(transform.m_position));

            glm::mat4 rx = glm::rotate(glm::mat4(1.f), glm::radians(transform.m_rotation.x), glm::vec3(1, 0, 0));
            glm::mat4 ry = glm::rotate(glm::mat4(1.f), glm::radians(transform.m_rotation.y), glm::vec3(0, 1, 0));
            glm::mat4 rz = glm::rotate(glm::mat4(1.f), glm::radians(transform.m_rotation.z), glm::vec3(0, 0, 1));
                
            glm::mat4 s = glm::scale(glm::mat4(1.f), glm::vec3(transform.m_scale));
                
            transform.modelMatrix = t * (rz * ry * rx) * s;
        }
    }

    void Scene::destroyActor(Actor actor)
    {
        if (actor.isValid())
        {
            m_registry.destroy(actor.getEntity());
        }
    }

    // Actor implementation
    entt::registry& Actor::getRegistry()
    {
        return m_scene->getRegistry();
    }

    const entt::registry& Actor::getRegistry() const
    {
        return m_scene->getRegistry();
    }

}

