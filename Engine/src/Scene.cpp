#include "Scene.h"
#include "Common/Common.h"

namespace ToyEngine
{
    Actor Scene::createActor()
    {
        entt::entity entity = m_registry.create();
        m_registry.emplace<Transform>(entity);
        m_registry.emplace<std::vector<PipelineHandle>>(entity);
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
    void Actor::registerPipeline(PipelineHandle handle)
    {
        auto& pipelines = m_scene->getRegistry().get<std::vector<PipelineHandle>>(m_entity);
        pipelines.push_back(handle);
    }

    bool Actor::hasPipeline(PipelineHandle handle) const
    {
        const auto& pipelines = m_scene->getRegistry().get<std::vector<PipelineHandle>>(m_entity);
        for (auto& h : pipelines)
        {
            if (h.index == handle.index && h.generation == handle.generation)
            {
                return true;
            }
        }
        return false;
    }

    entt::registry& Actor::getRegistry()
    {
        return m_scene->getRegistry();
    }

    const entt::registry& Actor::getRegistry() const
    {
        return m_scene->getRegistry();
    }

}

