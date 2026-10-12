#pragma once

#include <string>
#include <vector>
#include <volk.h>
#include <entt/entt.hpp>
#include "Common/Common.h"

#include "Supplier.h"
#include "GpuResources.h"
#include "ResourceManager.h"
#include <glm/glm.hpp>  
#include <glm/gtc/matrix_transform.hpp>  

namespace ToyEngine
{
    class World
    {
    public:
        World() = default;
        ~World() = default;

        Supplier createSupplier();
        void destroySupplier(Supplier actor);

        void update();
        
        FORCEINLINE entt::registry& getRegistry() { return m_registry; }
        FORCEINLINE const entt::registry& getRegistry() const { return m_registry; }

    private:
        entt::registry m_registry;
        
        // should create here an item that holds the prefiltered
        // supplies from the scene?
        // Something prepared for the "render thread", so we can just iterate over 
        // commands and draw 
    };
}
