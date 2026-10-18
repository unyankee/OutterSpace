#pragma once

#include <vector>

#include "Common/Common.h"
#include "ResourceManager.h"

namespace ToyEngine
{
    class World;

    class TransformSystem
    {
    public:
        void init(ResourceManager& resourceManager);
        void update(World& world, ResourceManager& resourceManager);

        BufferHandle getGpuBufferHandle() const { return m_gpuBufferHandle; }
        const std::vector<Transform>& getTransforms() const { return m_transforms; }

    private:
        BufferHandle m_gpuBufferHandle;
        std::vector<Transform> m_transforms;
        std::vector<GPUTransform> m_gpuTransforms;
    };
}