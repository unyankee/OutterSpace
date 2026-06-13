#pragma once

#include <entt/entt.hpp>
#include "Common/Common.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "Camera.h" // For Vec3

namespace ToyEngine
{
    class Scene;

    struct Transform
    {
        Vec3 m_position = {0, 0, 0};
        Vec3 m_rotation = {0, 0, 0};
        Vec3 m_scale = {1, 1, 1};
    };

    class Actor
    {
    public:
        Actor(entt::entity entity, Scene* scene)
            : m_entity(entity), m_scene(scene)
        {
        }

        void registerPipeline(PipelineHandle handle);
        bool hasPipeline(PipelineHandle handle) const;

        entt::registry& getRegistry();
        const entt::registry& getRegistry() const;

        template<typename T, typename... Args>
        TOY_FORCEINLINE T& addComponent(Args&&... args)
        {
            return getRegistry().emplace<T>(m_entity, std::forward<Args>(args)...);
        }

        template<typename T>
        TOY_FORCEINLINE T& getComponent()
        {
            return getRegistry().get<T>(m_entity);
        }

        template<typename T>
        TOY_FORCEINLINE const T& getComponent() const
        {
            return getRegistry().get<T>(m_entity);
        }

        template<typename T>
        TOY_FORCEINLINE bool hasComponent() const
        {
            return getRegistry().all_of<T>(m_entity);
        }

        template<typename T>
        TOY_FORCEINLINE void removeComponent()
        {
            getRegistry().remove<T>(m_entity);
        }

        TOY_FORCEINLINE bool isValid() const { return m_entity != entt::null && m_scene != nullptr; }
        TOY_FORCEINLINE entt::entity getEntity() const { return m_entity; }

    private:
        entt::entity m_entity = entt::null;
        Scene* m_scene = nullptr;
    };
}
