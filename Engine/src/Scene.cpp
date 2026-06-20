#include "Scene.h"
#include "Common/Common.h"

namespace ToyEngine
{
    
    Actor Scene::createActor()
    {
        entt::entity entity = m_registry.create();
        // Every Actor will be rendered, so we assign a transform to it
        m_registry.emplace<Transform>(entity);

        const uint32_t transformIndex = transformSystem.alloc();
        m_registry.emplace<TransformIndex>(entity, transformIndex);
        
        return Actor(entity, this);
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

