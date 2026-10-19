#pragma once

#include <volk.h>
#include <cstdint>

#include "GpuResources.h"
#include "ResourceManager.h"

namespace ToyEngine
{

    class PipelineManager
    {
    public:
        void init(const GpuContext& context, ResourceManager& resourceManager);
        void SetCameraBuffer(Buffer& cameraBuffer, ResourceManager& resourceManager);
        void AddTextureToDescriptorHeap(Texture& texture, ResourceManager& resourceManager);
        void bindDescriptorHeaps(VkCommandBuffer commandBuffer, ResourceManager& resourceManager) const;

        uint32_t getBufferDescriptorStride() const { return m_bufferDescriptorStride; }
        uint32_t getImageHeapOffset() const { return m_imageHeapOffset; }
        uint32_t getImageDescriptorStride() const { return m_imageDescriptorStride; }
        uint32_t getSamplerDescriptorStride() const { return m_samplerDescriptorStride; }

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

        VkPhysicalDeviceDescriptorHeapPropertiesEXT m_heapProperties{};

        BufferHandle m_resourceHeap;
        BufferHandle m_samplerHeap;
        DescriptorHeapLocation m_defaultSamplerLocation{};

        uint32_t m_imageDescriptorSize = 0;
        uint32_t m_bufferDescriptorSize = 0;
        uint32_t m_samplerDescriptorSize = 0;
        uint32_t m_bufferDescriptorStride = 0;
        uint32_t m_imageHeapOffset = 0;
        uint32_t m_imageDescriptorStride = 0;
        uint32_t m_samplerDescriptorStride = 0;
        uint32_t m_textureCount = 0;
    };

}
