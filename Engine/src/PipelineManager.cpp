#include "PipelineManager.h"

#include "Common/Common.h"

#include <cassert>
#include <stdexcept>

namespace
{
    VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment)
    {
        return (value + alignment - 1) & ~(alignment - 1);
    }
}

namespace ToyEngine
{

    void PipelineManager::init(const GpuContext& context, ResourceManager& resourceManager)
    {
        m_device = context.m_device;
        m_physicalDevice = context.m_physicalDevice;
        assert(m_device != VK_NULL_HANDLE);

        VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProperties{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT};
        VkPhysicalDeviceProperties2 properties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        properties.pNext = &heapProperties;
        vkGetPhysicalDeviceProperties2(m_physicalDevice, &properties);
        m_heapProperties = heapProperties;

        m_imageDescriptorSize = static_cast<uint32_t>(vkGetPhysicalDeviceDescriptorSizeEXT(
            m_physicalDevice, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE));
        m_bufferDescriptorSize = static_cast<uint32_t>(vkGetPhysicalDeviceDescriptorSizeEXT(
            m_physicalDevice, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
        m_samplerDescriptorSize = static_cast<uint32_t>(vkGetPhysicalDeviceDescriptorSizeEXT(
            m_physicalDevice, VK_DESCRIPTOR_TYPE_SAMPLER));
        m_imageDescriptorStride = static_cast<uint32_t>(alignUp(m_imageDescriptorSize,
            m_heapProperties.imageDescriptorAlignment));
        m_bufferDescriptorStride = static_cast<uint32_t>(alignUp(m_bufferDescriptorSize,
            m_heapProperties.bufferDescriptorAlignment));
        m_imageHeapOffset = static_cast<uint32_t>(alignUp(m_bufferDescriptorStride,
            m_heapProperties.imageDescriptorAlignment));
        m_samplerDescriptorStride = static_cast<uint32_t>(alignUp(m_samplerDescriptorSize,
            m_heapProperties.samplerDescriptorAlignment));

        constexpr uint32_t MaxHeapTextures = 1000;
        const VkDeviceSize resourceHeapBytes = alignUp(
            VkDeviceSize(m_imageHeapOffset) + VkDeviceSize(MaxHeapTextures) * m_imageDescriptorStride +
                m_heapProperties.minResourceHeapReservedRange,
            m_heapProperties.resourceHeapAlignment);

        const VkDeviceSize samplerHeapBytes = alignUp(
            VkDeviceSize(m_samplerDescriptorStride) + m_heapProperties.minSamplerHeapReservedRange,
            m_heapProperties.samplerHeapAlignment);

        if (resourceHeapBytes > m_heapProperties.maxResourceHeapSize ||
            samplerHeapBytes > m_heapProperties.maxSamplerHeapSize)
        {
            throw std::runtime_error("Configured descriptor heaps exceed device limits");
        }

        constexpr VkMemoryPropertyFlags HeapMemory = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        const VkBufferUsageFlags heapUsage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        m_resourceHeap = resourceManager.createBuffer(static_cast<uint32_t>(resourceHeapBytes), heapUsage, HeapMemory);
        m_samplerHeap = resourceManager.createBuffer(static_cast<uint32_t>(samplerHeapBytes), heapUsage, HeapMemory);

        // Adding default sampler
        Buffer* samplerHeap = resourceManager.getBuffer(m_samplerHeap);

        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        m_defaultSamplerLocation = {0, VK_DESCRIPTOR_TYPE_SAMPLER};
        VkHostAddressRangeEXT samplerDestination{
            static_cast<uint8_t*>(samplerHeap->m_data) + m_defaultSamplerLocation.m_offset,
            m_samplerDescriptorSize};

        VK_CHECK(vkWriteSamplerDescriptorsEXT(m_device, 1, &samplerInfo, &samplerDestination));
    }

    void PipelineManager::SetCameraBuffer(Buffer& cameraBuffer, ResourceManager& resourceManager)
    {
        cameraBuffer.m_descriptorHeapLocation = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER};
        const VkDeviceAddressRangeEXT addressRange{cameraBuffer.m_gpuAddress, cameraBuffer.m_size};
        VkResourceDescriptorInfoEXT resourceInfo{VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT};
        resourceInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        resourceInfo.data.pAddressRange = &addressRange;

        Buffer* resourceHeap = resourceManager.getBuffer(m_resourceHeap);
        VkHostAddressRangeEXT destination{
            static_cast<uint8_t*>(resourceHeap->m_data) + cameraBuffer.m_descriptorHeapLocation.m_offset,
            m_bufferDescriptorSize};
        VK_CHECK(vkWriteResourceDescriptorsEXT(m_device, 1, &resourceInfo, &destination));
    }

    void PipelineManager::AddTextureToDescriptorHeap(Texture& texture, ResourceManager& resourceManager)
    {
        constexpr uint32_t MaxHeapTextures = 1000;
        assert(m_textureCount < MaxHeapTextures);
        const uint32_t descriptorIndex = m_textureCount++;
        texture.m_bindlessIndex = descriptorIndex;
        texture.m_descriptorHeapLocation = {
            m_imageHeapOffset + descriptorIndex * m_imageDescriptorStride,
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE};

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = texture.m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = texture.m_format;
        viewInfo.subresourceRange.aspectMask = texture.m_aspect;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageDescriptorInfoEXT imageInfo{VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT};
        imageInfo.pView = &viewInfo;
        imageInfo.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkResourceDescriptorInfoEXT resourceInfo{VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT};
        resourceInfo.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        resourceInfo.data.pImage = &imageInfo;

        Buffer* resourceHeap = resourceManager.getBuffer(m_resourceHeap);
        uint8_t* descriptorBytes = static_cast<uint8_t*>(resourceHeap->m_data) +
            texture.m_descriptorHeapLocation.m_offset;

        VkHostAddressRangeEXT destination{descriptorBytes, m_imageDescriptorSize};
        VK_CHECK(vkWriteResourceDescriptorsEXT(m_device, 1, &resourceInfo, &destination));
    }

    void PipelineManager::bindDescriptorHeaps(VkCommandBuffer commandBuffer,
                                               ResourceManager& resourceManager) const
    {
        const Buffer* resourceHeap = resourceManager.getBuffer(m_resourceHeap);
        const Buffer* samplerHeap = resourceManager.getBuffer(m_samplerHeap);

        VkBindHeapInfoEXT resourceBinding{VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT};
        resourceBinding.heapRange = {resourceHeap->m_gpuAddress, resourceHeap->m_size};
        resourceBinding.reservedRangeOffset = resourceHeap->m_size - m_heapProperties.minResourceHeapReservedRange;
        resourceBinding.reservedRangeSize = m_heapProperties.minResourceHeapReservedRange;
        vkCmdBindResourceHeapEXT(commandBuffer, &resourceBinding);

        VkBindHeapInfoEXT samplerBinding{VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT};
        samplerBinding.heapRange = {samplerHeap->m_gpuAddress, samplerHeap->m_size};
        samplerBinding.reservedRangeOffset = samplerHeap->m_size - m_heapProperties.minSamplerHeapReservedRange;
        samplerBinding.reservedRangeSize = m_heapProperties.minSamplerHeapReservedRange;
        vkCmdBindSamplerHeapEXT(commandBuffer, &samplerBinding);
    }

}
