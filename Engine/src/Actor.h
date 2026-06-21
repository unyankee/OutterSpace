#pragma once

#include <entt/entt.hpp>
#include "Common/Common.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "Camera.h" // For Vec3

namespace ToyEngine
{
    class Scene;

    class Actor
    {
        friend class Scene;
    protected:
        Actor(entt::entity entity, Scene* scene)
            : m_entity(entity), m_scene(scene)
        {
        }
    public:

        entt::registry& getRegistry();
        const entt::registry& getRegistry() const;

        template<typename T, typename... Args>
        FORCEINLINE T& addComponent(Args&&... args)
        {
            return getRegistry().emplace<T>(m_entity, std::forward<Args>(args)...);
        }

        template<typename T>
        FORCEINLINE T& getComponent()
        {
            return getRegistry().get<T>(m_entity);
        }

        template<typename T>
        FORCEINLINE const T& getComponent() const
        {
            return getRegistry().get<T>(m_entity);
        }

        template<typename T>
        FORCEINLINE bool hasComponent() const
        {
            return getRegistry().all_of<T>(m_entity);
        }

        template<typename T>
        FORCEINLINE void removeComponent()
        {
            getRegistry().remove<T>(m_entity);
        }

        FORCEINLINE bool isValid() const { return m_entity != entt::null && m_scene != nullptr; }
        FORCEINLINE entt::entity getEntity() const { return m_entity; }
        FORCEINLINE uint32_t getTransformIndex() const { return getComponent<TransformIndex>().index; }
        
    private:
        entt::entity m_entity = entt::null;
        Scene* m_scene = nullptr;
    };
}
