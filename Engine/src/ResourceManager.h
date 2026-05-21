#pragma once

#include <volk.h>
#include <cstdint>
#include <vector>

#include "GpuResources.h"

namespace ToyEngine
{
    constexpr uint32_t InvalidResourceIndex = 0xffffffffu;

    struct ResourceHandle
    {
        // Index is used for the current slot being used, as for generation,
        // is a way to control already deleted resources.
        // Preventing them from accessing resources that should not be bound
        uint32_t index = InvalidResourceIndex;
        uint32_t generation = 0;

        bool isValid() const { return index != InvalidResourceIndex; }
    };

    struct BufferHandle : ResourceHandle
    {
        BufferHandle() = default;
        BufferHandle(uint32_t index, uint32_t generation) : ResourceHandle{index, generation} {}
    };

    struct TextureHandle : ResourceHandle
    {
        TextureHandle() = default;
        TextureHandle(uint32_t index, uint32_t generation) : ResourceHandle{index, generation} {}
    };

    struct RenderTargetHandle : ResourceHandle
    {
        RenderTargetHandle() = default;
        RenderTargetHandle(uint32_t index, uint32_t generation) : ResourceHandle{index, generation} {}
    };

    struct PipelineHandle : ResourceHandle
    {
        PipelineHandle() = default;
        PipelineHandle(uint32_t index, uint32_t generation) : ResourceHandle{index, generation} {}
    };

    class Pipeline;
    struct PipelineConfig;

    class ResourceManager
    {
    public:
        ResourceManager() = default;

        ~ResourceManager();

        void init(GpuContext& ctx);

        void cleanup();

        BufferHandle createBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void* initialData = nullptr);
        Buffer* getBuffer(BufferHandle handle);
        const Buffer* getBuffer(BufferHandle handle) const;
        void destroyBuffer(BufferHandle handle);

        TextureHandle createTexture(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);
        TextureHandle loadTexture(const char* path);
        Texture* getTexture(TextureHandle handle);
        const Texture* getTexture(TextureHandle handle) const;
        void destroyTexture(TextureHandle handle);

        RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect);
        RenderTarget* getRenderTarget(RenderTargetHandle handle);
        const RenderTarget* getRenderTarget(RenderTargetHandle handle) const;
        void destroyRenderTarget(RenderTargetHandle handle);

        PipelineHandle createPipeline(const PipelineConfig& config, VkDescriptorSetLayout descriptorLayout);
        PipelineHandle createPipeline(const PipelineConfig& config, const std::vector<VkDescriptorSetLayout>& descriptorLayouts);
        Pipeline* getPipeline(PipelineHandle handle);
        const Pipeline* getPipeline(PipelineHandle handle) const;
        void destroyPipeline(PipelineHandle handle);

        VkSemaphore createSemaphore(uint64_t initialValue = 0);
        VkSemaphore createBinarySemaphore();
        void destroySemaphore(VkSemaphore semaphore);

        VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
        void destroyCommandPool(VkCommandPool commandPool);

    private:
        template <typename T>
        struct ResourceSlot
        {
            T resource{};
            uint32_t generation = 1;
            bool alive = false;
        };

        GpuContext* m_ctx = nullptr;

        std::vector<ResourceSlot<Buffer>> m_buffers;
        std::vector<ResourceSlot<Texture>> m_textures;
        std::vector<ResourceSlot<RenderTarget>> m_renderTargets;
        std::vector<ResourceSlot<Pipeline>> m_pipelines;

        std::vector<uint32_t> m_freeBuffers;
        std::vector<uint32_t> m_freeTextures;
        std::vector<uint32_t> m_freeRenderTargets;
        std::vector<uint32_t> m_freePipelines;

        std::vector<VkSemaphore> m_semaphores;
        std::vector<VkCommandPool> m_commandPools;
    };

}
