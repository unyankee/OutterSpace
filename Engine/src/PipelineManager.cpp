#include "PipelineManager.h"
#include "Common/Common.h"
#include <iostream>
#include <cassert>

namespace ToyEngine
{
    void PipelineManager::init(VkDevice device)
    {
        m_device = device;
        assert(m_device != VK_NULL_HANDLE);

        // Setup global descriptor set layout
        VkDescriptorSetLayoutBinding bindings[2] = {};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        bindings[0].descriptorCount = 1000;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        bindings[1].descriptorCount = 10;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorBindingFlags bindingFlags[2] = {
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT
        };

        VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
        };
        extendedInfo.bindingCount = 2;
        extendedInfo.pBindingFlags = bindingFlags;

        VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.pNext = &extendedInfo;
        layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        layoutInfo.bindingCount = 2;
        layoutInfo.pBindings = bindings;

        VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &globalBindlessLayout));

        setupGlobalDescriptorSet();
    }

    void PipelineManager::cleanup()
    {
        if (bindlessPool) vkDestroyDescriptorPool(m_device, bindlessPool, nullptr);
        if (globalBindlessLayout) vkDestroyDescriptorSetLayout(m_device, globalBindlessLayout, nullptr);
    }

    void PipelineManager::AddTextureToGlobalDescriptorSet(Texture& texture)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture.view;
        imageInfo.sampler = nullptr;

        uint32_t currentSlot = m_textureCount++;
        texture.bindlessIndex = currentSlot;

        VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorWrite.dstSet = globalBindlessDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = currentSlot;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

        // Default sampler
        static VkSampler linearSampler = VK_NULL_HANDLE;
        if (linearSampler == VK_NULL_HANDLE)
        {
            VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &linearSampler));

            VkDescriptorImageInfo samplerInfoWrite{};
            samplerInfoWrite.sampler = linearSampler;

            VkWriteDescriptorSet samplerWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            samplerWrite.dstSet = globalBindlessDescriptorSet;
            samplerWrite.dstBinding = 1;
            samplerWrite.dstArrayElement = 0;
            samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            samplerWrite.descriptorCount = 1;
            samplerWrite.pImageInfo = &samplerInfoWrite;

            vkUpdateDescriptorSets(m_device, 1, &samplerWrite, 0, nullptr);
        }
    }

    void PipelineManager::setupGlobalDescriptorSet()
    {
        VkDescriptorPoolSize poolSizes[2] = {
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 10}
        };

        VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;

        VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &bindlessPool));

        VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = bindlessPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &globalBindlessLayout;

        VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &globalBindlessDescriptorSet));
    }
}
