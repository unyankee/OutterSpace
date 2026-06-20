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
    struct TransformSystem
    {
        std::vector<Transform> TransformsData;
        std::vector<uint32_t> freeList;
        // should check against MaxTransformsPerScene too... more shortcuts
        uint32_t alloc()
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

        void free(uint32_t idx)
        {
            freeList.push_back(idx);
        }

        void update() {
            for (Transform& transform : TransformsData) {
                glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(transform.m_position));

                glm::mat4 rx = glm::rotate(glm::mat4(1.f), glm::radians(transform.m_rotation.x), glm::vec3(1, 0, 0));
                glm::mat4 ry = glm::rotate(glm::mat4(1.f), glm::radians(transform.m_rotation.y), glm::vec3(0, 1, 0));
                glm::mat4 rz = glm::rotate(glm::mat4(1.f), glm::radians(transform.m_rotation.z), glm::vec3(0, 0, 1));
                
                glm::mat4 s = glm::scale(glm::mat4(1.f), glm::vec3(transform.m_scale));
                
                transform.modelMatrix = t * (rz * ry * rx) * s;
            }
        }
    };

    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        Actor createActor();
        void destroyActor(Actor actor);

        FORCEINLINE entt::registry& getRegistry() { return m_registry; }
        FORCEINLINE const entt::registry& getRegistry() const { return m_registry; }

        TransformSystem transformSystem;
    private:
        entt::registry m_registry;
    };
}
