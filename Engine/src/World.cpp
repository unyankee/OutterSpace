#include "World.h"
#include "Common/Common.h"

namespace ToyEngine
{
    
    Supplier World::createSupplier()
    {
        entt::entity entity = m_registry.create();
        Supplier supplier {entity, this};
        Transform transform;
        supplier.setComponent(transform);
        
        return supplier;        
    }

    void World::update()
    {
        
        
    }

    void World::destroySupplier(Supplier actor)
    {
        if (actor.isValid())
        {
            // this is not as simple as removing it...
            //m_registry.destroy(actor.getEntity());
        }
    }

    // Actor implementation
    entt::registry& Supplier::getRegistry()
    {
        return m_scene->getRegistry();
    }

    const entt::registry& Supplier::getRegistry() const
    {
        return m_scene->getRegistry();
    }

}

