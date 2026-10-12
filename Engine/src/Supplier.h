#pragma once

#include <entt/entt.hpp>
#include "Common/Common.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "Camera.h" // For Vec3

namespace ToyEngine
{
    class World;

    class Supplier
    {
        friend class World;
    protected:
        Supplier(entt::entity entity, World* scene)
            : m_entity(entity), m_scene(scene)
        {
        }
    public:
        template<typename T>
        FORCEINLINE T& getComponent()
        {
            return getRegistry().get<T>(m_entity);
        }
        
        template<typename T>
        FORCEINLINE void setComponent(T& t)
        {
            getRegistry().emplace_or_replace<T>(m_entity, t);
        }

        template<typename T>
        FORCEINLINE void addComponent(T& t)
        {
            getRegistry().emplace<T>(m_entity, t);
        }

        FORCEINLINE bool isValid() const { return m_entity != entt::null && m_scene != nullptr; }
    private:
        entt::registry& getRegistry();
        const entt::registry& getRegistry() const;
        
        entt::entity m_entity = entt::null;
        World* m_scene = nullptr;
    };
}
