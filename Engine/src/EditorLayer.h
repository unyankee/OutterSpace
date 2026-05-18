#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <imgui.h>
#include "GpuResources.h"
#include "Pipeline.h"

namespace ToyEngine
{
    class EditorLayer
    {
    public:
        void init(const GpuContext& ctx, GLFWwindow* window, VkFormat colorFormat);
        void beginFrame();
        void render(VkCommandBuffer cmd, uint32_t width, uint32_t height);
        void destroy(const GpuContext& ctx);

    private:
        void createPipeline(const GpuContext& ctx, VkFormat colorFormat);
        void createFontAtlas(const GpuContext& ctx);
        void updateBuffers(const GpuContext& ctx, ImDrawData* drawData);

        Pipeline m_pipeline;
        Texture m_fontTexture;

        Buffer m_vertexBuffer;
        Buffer m_indexBuffer;

        VkDescriptorSetLayout m_textureLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_samplerLayout = VK_NULL_HANDLE;
        VkDescriptorSet m_fontDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSet m_samplerDescriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkSampler m_sampler = VK_NULL_HANDLE;

        GLFWwindow* m_window = nullptr;
        GpuContext m_ctx;
    };
}
