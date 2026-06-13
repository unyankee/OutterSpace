#pragma once

#include <string>
#include <vector>
#include <volk.h>
#include <entt/entt.hpp>
#include "Common/Common.h"

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

        TOY_FORCEINLINE entt::registry& getRegistry() { return m_registry; }
        TOY_FORCEINLINE const entt::registry& getRegistry() const { return m_registry; }

    private:
        entt::registry m_registry;
    };
}
