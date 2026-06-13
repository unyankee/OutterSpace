#pragma once

#include <string>
#include <vector>
#include <volk.h>
#include <entt/entt.hpp>

#include "Actor.h"
#include "GpuResources.h"
#include "ResourceManager.h"

namespace ToyEngine
{
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Actor createActor();
        void destroyActor(Actor actor);

        entt::registry& getRegistry() { return m_registry; }
        const entt::registry& getRegistry() const { return m_registry; }

    private:
        entt::registry m_registry;
    };
}
