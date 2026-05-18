#pragma once

#include <vector>
#include <algorithm>
#include "Mesh.h"
#include "Pipeline.h"

namespace ToyEngine
{
    struct Transform
    {
        Vec3 position = {0, 0, 0};
        Vec3 rotation = {0, 0, 0};
        Vec3 scale = {1, 1, 1};
    };

    class Actor
    {
    public:
        Actor(Mesh* mesh) : m_mesh(mesh)
        {
        }

        void registerPipeline(Pipeline* pipeline)
        {
            m_registered_pipelines.push_back(pipeline);
        }

        bool hasPipeline(Pipeline* pipeline) const
        {
            return std::find(m_registered_pipelines.begin(), m_registered_pipelines.end(), pipeline) != m_registered_pipelines.end();
        }

        void draw(VkCommandBuffer cmd, Pipeline* pipeline, VkDeviceAddress cameraAddress, uint32_t vertexBufferAddress,
                  uint32_t textureIndex, uint32_t samplerIndex)
        {
            // This will be expanded when we have proper push constant data per actor
            // For now, it just binds the index buffer and draws
        }

        Mesh* getMesh() const { return m_mesh; }
        Transform& getTransform() { return m_transform; }

    private:
        Mesh* m_mesh;
        Transform m_transform;
        std::vector<Pipeline*> m_registered_pipelines;
    };
}
