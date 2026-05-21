#pragma once

#include <vector>
#include <algorithm>

#include "Mesh.h"
#include "ResourceManager.h"

namespace ToyEngine
{

    struct Transform
    {
        Vec3 m_position = {0, 0, 0};
        Vec3 m_rotation = {0, 0, 0};
        Vec3 m_scale = {1, 1, 1};
    };

    class Actor
    {
    public:
        Actor(Mesh* mesh)
            : m_mesh(mesh)
        {
        }

        void registerPipeline(PipelineHandle handle)
        {
            m_registered_pipelines.push_back(handle);
        }

        bool hasPipeline(PipelineHandle handle) const
        {
            for (auto& h : m_registered_pipelines)
            {
                if (h.index == handle.index && h.generation == handle.generation)
                {
                    return true;
                }
            }
            return false;
        }

        void draw(VkCommandBuffer cmd, PipelineHandle handle, VkDeviceAddress cameraAddress, uint32_t vertexBufferAddress,
                  uint32_t textureIndex, uint32_t samplerIndex)
        {
            
        }

        Mesh* getMesh() const
        {
            return m_mesh;
        }

        Transform& getTransform()
        {
            return m_transform;
        }

    private:
        Mesh* m_mesh;
        Transform m_transform;
        std::vector<PipelineHandle> m_registered_pipelines;
    };

}
