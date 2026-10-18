#include "TransformSystem.h"

#include "World.h"
#include <glm/gtc/matrix_transform.hpp>

namespace ToyEngine
{
    static_assert(sizeof(GPUTransform) == sizeof(float) * 12);
    void TransformSystem::init(ResourceManager& resourceManager)
    {
        m_gpuBufferHandle = resourceManager.createBuffer(sizeof(GPUTransform) * MaxTransformsPerScene,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_transforms.reserve(MaxTransformsPerScene);
        m_gpuTransforms.reserve(MaxTransformsPerScene);
    }

    void TransformSystem::update(World& world, ResourceManager& resourceManager)
    {
        m_transforms.clear();
        m_gpuTransforms.clear();

        auto view = world.getRegistry().view<Transform>();
        for (const auto& [entity, transform] : view.each())
        {
            const uint32_t transformIndex = static_cast<uint32_t>(m_transforms.size());
            if (transformIndex >= MaxTransformsPerScene)
            {
                assert(false && "Scene exceeds MaxTransformsPerScene");
                break;
            }

            const glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(transform.m_position));
            const glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(transform.m_rotation.x), glm::vec3(1, 0, 0));
            const glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(transform.m_rotation.y), glm::vec3(0, 1, 0));
            const glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(transform.m_rotation.z), glm::vec3(0, 0, 1));
            const glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(transform.m_scale));
            transform.modelMatrix = translation * (rotationZ * rotationY * rotationX) * scale;

            m_transforms.push_back(transform);
            world.getRegistry().emplace_or_replace<TransformID>(entity, TransformID{transformIndex});

            GPUTransform gpuTransform{};
            gpuTransform.modelMatrix = glm::mat4x3(transform.modelMatrix);
            m_gpuTransforms.push_back(gpuTransform);
        }

        if (!m_gpuTransforms.empty())
        {
            Buffer* gpuBuffer = resourceManager.getBuffer(m_gpuBufferHandle);
            gpuBuffer->copyDataToBuffer(m_gpuTransforms.data(),
                static_cast<uint32_t>(m_gpuTransforms.size() * sizeof(GPUTransform)));
        }
    }

}
