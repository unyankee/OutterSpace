#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <imgui.h>

#include "GpuResources.h"
#include "Pipeline.h"
#include "ResourceManager.h"
#include "PipelineManager.h"

namespace ToyEngine
{

    class EditorLayer
    {
    public:
        void init(const GpuContext& ctx, ResourceManager* resourceManager, PipelineManager* pipelineManager,
                  GLFWwindow* window, VkFormat colorFormat);

        void beginFrame();

        void render(VkCommandBuffer cmd, uint32_t width, uint32_t height);

        void destroy();

        const PipelineHandle& GetPipeline() const {return m_pipeline;};

    private:
        void createPipeline(const GpuContext& ctx, VkFormat colorFormat, PipelineManager& pipelineManager);

        void createFontAtlas(const GpuContext& ctx, PipelineManager& pipelineManager);

        // that way will enforce the proper constructor (since I want config to be passed on constructor time)
        PipelineHandle m_pipeline;
        TextureHandle m_fontTexture;

        BufferHandle m_vertexBuffer;
        BufferHandle m_indexBuffer;


        GLFWwindow* m_window = nullptr;
        const GpuContext* m_ctx = nullptr;
        ResourceManager* m_resourceManager = nullptr;
    };

}
