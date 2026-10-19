#include "EditorLayer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>

#include "Common/Common.h"
#include <vector>

namespace ToyEngine
{

    void EditorLayer::init(const GpuContext& ctx, ResourceManager* resourceManager,
                           PipelineManager* pipelineManager, GLFWwindow* window, VkFormat colorFormat)
    {
        m_window = window;
        m_ctx = &ctx;
        m_resourceManager = resourceManager;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);

        createFontAtlas(ctx, *pipelineManager);
        createPipeline(ctx, colorFormat, *pipelineManager);

        m_vertexBuffer = m_resourceManager->createBuffer(1024 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_indexBuffer = m_resourceManager->createBuffer(1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void EditorLayer::beginFrame()
    {
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void EditorLayer::render(VkCommandBuffer cmd, uint32_t width, uint32_t height)
    {
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        if (!drawData || drawData->TotalVtxCount == 0)
        {
            return;
        }

        // Update vertex/index buffers
        size_t vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        size_t indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
        Buffer* vertexBuffer = m_resourceManager->getBuffer(m_vertexBuffer);
        Buffer* indexBuffer = m_resourceManager->getBuffer(m_indexBuffer);

        if (vertexBuffer->m_size < vertexSize)
        {
            m_resourceManager->destroyBuffer(m_vertexBuffer);
            m_vertexBuffer = m_resourceManager->createBuffer((uint32_t)vertexSize * 2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            vertexBuffer = m_resourceManager->getBuffer(m_vertexBuffer);
        }

        if (indexBuffer->m_size < indexSize)
        {
            m_resourceManager->destroyBuffer(m_indexBuffer);
            m_indexBuffer = m_resourceManager->createBuffer((uint32_t)indexSize * 2, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            indexBuffer = m_resourceManager->getBuffer(m_indexBuffer);
        }

        ImDrawVert* vtxDst = (ImDrawVert*)vertexBuffer->map(*m_ctx);
        ImDrawIdx* idxDst = (ImDrawIdx*)indexBuffer->map(*m_ctx);

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmdList->VtxBuffer.Size;
            idxDst += cmdList->IdxBuffer.Size;
        }

        vertexBuffer->unmap(*m_ctx);
        indexBuffer->unmap(*m_ctx);

        // Bind pipeline and descriptor sets
        Pipeline* pipeline = m_resourceManager->getPipeline(m_pipeline);
        EditorPipelineLayout pc{};
        pc.scale[0] = 2.0f / drawData->DisplaySize.x;
        pc.scale[1] = 2.0f / drawData->DisplaySize.y;
        pc.translate[0] = -1.0f - drawData->DisplayPos.x * pc.scale[0];
        pc.translate[1] = -1.0f - drawData->DisplayPos.y * pc.scale[1];
        pc.VertexDataPtr = vertexBuffer->m_gpuAddress;

        if (pipeline)
        {
            VkViewport viewport = {0, 0, (float)width, (float)height, 0, 1};
            vkCmdSetViewport(cmd, 0, 1, &viewport);
        }

        VkDeviceSize offset = 0;
        vkCmdBindIndexBuffer(cmd, indexBuffer->m_buffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

        int vtxOffset = 0;
        int idxOffset = 0;

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];

            for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];

                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmdList, pcmd);
                }
                else
                {
                    VkRect2D scissor;
                    scissor.offset.x = (int32_t)(pcmd->ClipRect.x - drawData->DisplayPos.x) > 0 ? (int32_t)(pcmd->ClipRect.x - drawData->DisplayPos.x) : 0;
                    scissor.offset.y = (int32_t)(pcmd->ClipRect.y - drawData->DisplayPos.y) > 0 ? (int32_t)(pcmd->ClipRect.y - drawData->DisplayPos.y) : 0;
                    scissor.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
                    scissor.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);

                    vkCmdSetScissor(cmd, 0, 1, &scissor);

                    pc.TextureIndex = static_cast<uint32_t>(pcmd->GetTexID() - 1);
                    VkPushDataInfoEXT pushDataInfo{VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT};
                    pushDataInfo.offset = 0;
                    pushDataInfo.data = {&pc, sizeof(pc)};
                    vkCmdPushDataEXT(cmd, &pushDataInfo);
                    
                    vkCmdDrawIndexed(cmd, pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset, 0);
                }
            }

            vtxOffset += cmdList->VtxBuffer.Size;
            idxOffset += cmdList->IdxBuffer.Size;
        }
    }

    void EditorLayer::destroy()
    {
        m_resourceManager->destroyPipeline(m_pipeline);
        
        m_resourceManager->destroyTexture(m_fontTexture);
        m_resourceManager->destroyBuffer(m_vertexBuffer);
        m_resourceManager->destroyBuffer(m_indexBuffer);

        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void EditorLayer::createPipeline(const GpuContext& ctx, VkFormat colorFormat,
                                     PipelineManager& pipelineManager)
    {
        PipelineConfig config{};
        // We expect shaders to be pre-compiled to .spv
        config.m_vertexShader = Pipeline::loadShader(ctx.m_device, "../extern/imgui/glsl_shader.vert.spv");
        config.m_fragmentShader = Pipeline::loadShader(ctx.m_device, "../extern/imgui/glsl_shader.frag.spv");
        config.m_colorFormat = colorFormat;
        config.m_depthTest = false;
        config.m_depthWrite = false;
        config.m_blending = true;
        config.m_cullMode = VK_CULL_MODE_NONE;

        config.m_pushDataStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        config.m_imageHeapOffset = pipelineManager.getImageHeapOffset();
        config.m_imageDescriptorStride = pipelineManager.getImageDescriptorStride();
        config.m_samplerDescriptorStride = pipelineManager.getSamplerDescriptorStride();

        m_pipeline = m_resourceManager->createPipeline(config);
    }

    void EditorLayer::createFontAtlas(const GpuContext& ctx, PipelineManager& pipelineManager)
    {
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        m_fontTexture = m_resourceManager->createTexture(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        Texture* fontTexture = m_resourceManager->getTexture(m_fontTexture);
        fontTexture->uploadData(ctx, pixels, width * height * 4);

        pipelineManager.AddTextureToDescriptorHeap(*fontTexture, *m_resourceManager);
        io.Fonts->SetTexID(static_cast<ImTextureID>(fontTexture->m_bindlessIndex + 1));
    }

}
