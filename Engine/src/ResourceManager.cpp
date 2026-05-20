#include "ResourceManager.h"
#include "Common/Common.h"

#include <utility>

namespace ToyEngine
{

    ResourceManager::~ResourceManager()
    {
        cleanup();
    }

    void ResourceManager::init(GpuContext& ctx)
    {
        m_ctx = &ctx;
    }

    void ResourceManager::cleanup()
    {
        if (!m_ctx)
        {
            return;
        }

        for (auto& slot : m_buffers)
        {
            if (slot.alive)
            {
                slot.resource.destroy(*m_ctx);
                slot.alive = false;
                ++slot.generation;
            }
        }
        m_buffers.clear();
        m_freeBuffers.clear();

        for (auto& slot : m_textures)
        {
            if (slot.alive)
            {
                slot.resource.destroy(*m_ctx);
                slot.alive = false;
                ++slot.generation;
            }
        }
        m_textures.clear();
        m_freeTextures.clear();

        for (auto& slot : m_renderTargets)
        {
            if (slot.alive)
            {
                slot.resource.destroy(*m_ctx);
                slot.alive = false;
                ++slot.generation;
            }
        }
        m_renderTargets.clear();
        m_freeRenderTargets.clear();

        for (auto fence : m_fences)
        {
            if (fence != VK_NULL_HANDLE)
            {
                vkDestroyFence(m_ctx->m_device, fence, nullptr);
            }
        }
        m_fences.clear();

        for (auto semaphore : m_semaphores)
        {
            if (semaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_ctx->m_device, semaphore, nullptr);
            }
        }
        m_semaphores.clear();

        for (auto pool : m_commandPools)
        {
            if (pool != VK_NULL_HANDLE)
            {
                vkDestroyCommandPool(m_ctx->m_device, pool, nullptr);
            }
        }
        m_commandPools.clear();
    }

    BufferHandle ResourceManager::createBuffer(uint32_t size, VkBufferUsageFlags usage,
                                              VkMemoryPropertyFlags properties, const void* initialData)
    {
        uint32_t index = 0;
        if (!m_freeBuffers.empty())
        {
            index = m_freeBuffers.back();
            m_freeBuffers.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(m_buffers.size());
            m_buffers.emplace_back();
        }

        auto& slot = m_buffers[index];
        slot.resource.create(*m_ctx, size, usage, properties, initialData);
        slot.alive = true;
        return {index, slot.generation};
    }

    Buffer* ResourceManager::getBuffer(BufferHandle handle)
    {
        if (!handle.isValid() || handle.index >= m_buffers.size())
        {
            return nullptr;
        }

        auto& slot = m_buffers[handle.index];
        if (!slot.alive || slot.generation != handle.generation)
        {
            return nullptr;
        }

        return &slot.resource;
    }

    const Buffer* ResourceManager::getBuffer(BufferHandle handle) const
    {
        return const_cast<ResourceManager*>(this)->getBuffer(handle);
    }

    void ResourceManager::destroyBuffer(BufferHandle handle)
    {
        Buffer* buffer = getBuffer(handle);
        if (!buffer)
        {
            return;
        }

        auto& slot = m_buffers[handle.index];
        buffer->destroy(*m_ctx);
        slot.alive = false;
        ++slot.generation;
        m_freeBuffers.push_back(handle.index);
    }

    TextureHandle ResourceManager::createTexture(uint32_t width, uint32_t height, VkFormat format,
                                                VkImageUsageFlags usage, VkImageAspectFlags aspect)
    {
        uint32_t index = 0;
        if (!m_freeTextures.empty())
        {
            index = m_freeTextures.back();
            m_freeTextures.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(m_textures.size());
            m_textures.emplace_back();
        }

        auto& slot = m_textures[index];
        slot.resource.create(*m_ctx, width, height, format, usage, aspect);
        slot.alive = true;
        return {index, slot.generation};
    }

    TextureHandle ResourceManager::loadTexture(const char* path)
    {
        uint32_t index = 0;
        if (!m_freeTextures.empty())
        {
            index = m_freeTextures.back();
            m_freeTextures.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(m_textures.size());
            m_textures.emplace_back();
        }

        auto& slot = m_textures[index];
        slot.resource.load(*m_ctx, path);
        slot.alive = true;
        return {index, slot.generation};
    }

    Texture* ResourceManager::getTexture(TextureHandle handle)
    {
        if (!handle.isValid() || handle.index >= m_textures.size())
        {
            return nullptr;
        }

        auto& slot = m_textures[handle.index];
        if (!slot.alive || slot.generation != handle.generation)
        {
            return nullptr;
        }

        return &slot.resource;
    }

    const Texture* ResourceManager::getTexture(TextureHandle handle) const
    {
        return const_cast<ResourceManager*>(this)->getTexture(handle);
    }

    void ResourceManager::destroyTexture(TextureHandle handle)
    {
        Texture* texture = getTexture(handle);
        if (!texture)
        {
            return;
        }

        auto& slot = m_textures[handle.index];
        texture->destroy(*m_ctx);
        slot.alive = false;
        ++slot.generation;
        m_freeTextures.push_back(handle.index);
    }

    RenderTargetHandle ResourceManager::createRenderTarget(uint32_t width, uint32_t height, VkFormat format,
                                                          VkImageUsageFlags usage, VkImageAspectFlags aspect)
    {
        uint32_t index = 0;
        if (!m_freeRenderTargets.empty())
        {
            index = m_freeRenderTargets.back();
            m_freeRenderTargets.pop_back();
        }
        else
        {
            index = static_cast<uint32_t>(m_renderTargets.size());
            m_renderTargets.emplace_back();
        }

        auto& slot = m_renderTargets[index];
        slot.resource.create(*m_ctx, width, height, format, usage, aspect);
        slot.alive = true;
        return {index, slot.generation};
    }

    RenderTarget* ResourceManager::getRenderTarget(RenderTargetHandle handle)
    {
        if (!handle.isValid() || handle.index >= m_renderTargets.size())
        {
            return nullptr;
        }

        auto& slot = m_renderTargets[handle.index];
        if (!slot.alive || slot.generation != handle.generation)
        {
            return nullptr;
        }

        return &slot.resource;
    }

    const RenderTarget* ResourceManager::getRenderTarget(RenderTargetHandle handle) const
    {
        return const_cast<ResourceManager*>(this)->getRenderTarget(handle);
    }

    void ResourceManager::destroyRenderTarget(RenderTargetHandle handle)
    {
        RenderTarget* rt = getRenderTarget(handle);
        if (!rt)
        {
            return;
        }

        auto& slot = m_renderTargets[handle.index];
        rt->destroy(*m_ctx);
        slot.alive = false;
        ++slot.generation;
        m_freeRenderTargets.push_back(handle.index);
    }

    VkFence ResourceManager::createFence(bool signaled)
    {
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        if (signaled)
        {
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VkFence fence = VK_NULL_HANDLE;
        VK_CHECK(vkCreateFence(m_ctx->m_device, &fenceInfo, nullptr, &fence));
        m_fences.push_back(fence);

        return fence;
    }

    void ResourceManager::destroyFence(VkFence fence)
    {
        if (fence == VK_NULL_HANDLE)
        {
            return;
        }

        for (uint32_t i = 0; i < m_fences.size(); ++i)
        {
            if (m_fences[i] == fence)
            {
                vkDestroyFence(m_ctx->m_device, fence, nullptr);
                m_fences[i] = m_fences.back();
                m_fences.pop_back();
                return;
            }
        }
    }

    VkSemaphore ResourceManager::createSemaphore()
    {
        VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VK_CHECK(vkCreateSemaphore(m_ctx->m_device, &semaphoreInfo, nullptr, &semaphore));
        m_semaphores.push_back(semaphore);

        return semaphore;
    }

    void ResourceManager::destroySemaphore(VkSemaphore semaphore)
    {
        if (semaphore == VK_NULL_HANDLE)
        {
            return;
        }

        for (uint32_t i = 0; i < m_semaphores.size(); ++i)
        {
            if (m_semaphores[i] == semaphore)
            {
                vkDestroySemaphore(m_ctx->m_device, semaphore, nullptr);
                m_semaphores[i] = m_semaphores.back();
                m_semaphores.pop_back();
                return;
            }
        }
    }

    VkCommandPool ResourceManager::createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
    {
        VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = flags;

        VkCommandPool pool = VK_NULL_HANDLE;
        VK_CHECK(vkCreateCommandPool(m_ctx->m_device, &poolInfo, nullptr, &pool));
        m_commandPools.push_back(pool);

        return pool;
    }

    void ResourceManager::destroyCommandPool(VkCommandPool commandPool)
    {
        if (commandPool == VK_NULL_HANDLE)
        {
            return;
        }

        for (uint32_t i = 0; i < m_commandPools.size(); ++i)
        {
            if (m_commandPools[i] == commandPool)
            {
                vkDestroyCommandPool(m_ctx->m_device, commandPool, nullptr);
                m_commandPools[i] = m_commandPools.back();
                m_commandPools.pop_back();
                return;
            }
        }
    }

}
