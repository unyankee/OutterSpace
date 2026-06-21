#pragma once

#include <string>
#include <vector>
#include <volk.h>
#include <entt/entt.hpp>
#include "Common/Common.h"

#include "Actor.h"
#include "GpuResources.h"
#include "ResourceManager.h"
#include <glm/glm.hpp>  
#include <glm/gtc/matrix_transform.hpp>  

namespace ToyEngine
{
    struct TransformManager
    {
        std::vector<Transform> TransformsData;
        std::vector<uint32_t> freeList;

        Transform& getTransform(Actor& actor);
        uint32_t alloc();
        void free(uint32_t idx);
        void update();
    };

    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Actor createActor();
        void destroyActor(Actor actor);

        void update();
        
        FORCEINLINE entt::registry& getRegistry() { return m_registry; }
        FORCEINLINE const entt::registry& getRegistry() const { return m_registry; }

        TransformManager transformSystem;
    private:
        entt::registry m_registry;

    };
}
