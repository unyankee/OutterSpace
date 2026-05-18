#include "EditorLayer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "Common/Common.h"
#include <vector>

namespace ToyEngine
{
    void EditorLayer::init(const GpuContext& ctx, GLFWwindow* window, VkFormat colorFormat)
    {
        m_window = window;
        m_ctx = ctx;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Seems missing in this version

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);

        createPipeline(ctx, colorFormat);
        createFontAtlas(ctx);

        m_vertexBuffer.create(ctx, 1024 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_indexBuffer.create(ctx, 1024 * 1024, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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
            return;

        // Update vertex/index buffers
        size_t vertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
        size_t indexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

        if (m_vertexBuffer.size < vertexSize) m_vertexBuffer.create(m_ctx, (uint32_t)vertexSize * 2, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (m_indexBuffer.size < indexSize) m_indexBuffer.create(m_ctx, (uint32_t)indexSize * 2, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        ImDrawVert* vtxDst = (ImDrawVert*)m_vertexBuffer.map(m_ctx);
        ImDrawIdx* idxDst = (ImDrawIdx*)m_indexBuffer.map(m_ctx);

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtxDst += cmdList->VtxBuffer.Size;
            idxDst += cmdList->IdxBuffer.Size;
        }

        m_vertexBuffer.unmap(m_ctx);
        m_indexBuffer.unmap(m_ctx);

        // Bind pipeline and descriptor sets
        m_pipeline.bind(cmd);

        VkDescriptorSet sets[] = { m_fontDescriptorSet, m_samplerDescriptorSet };
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.getLayout(), 0, 2, sets, 0, nullptr);

        VkViewport viewport = { 0, 0, (float)width, (float)height, 0, 1 };
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        struct PushConstants {
            float scale[2];
            float translate[2];
        } pc;
        pc.scale[0] = 2.0f / drawData->DisplaySize.x;
        pc.scale[1] = 2.0f / drawData->DisplaySize.y;
        pc.translate[0] = -1.0f - drawData->DisplayPos.x * pc.scale[0];
        pc.translate[1] = -1.0f - drawData->DisplayPos.y * pc.scale[1];

        vkCmdPushConstants(cmd, m_pipeline.getLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(cmd, m_indexBuffer.buffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

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

                    vkCmdDrawIndexed(cmd, pcmd->ElemCount, 1, pcmd->IdxOffset + idxOffset, pcmd->VtxOffset + vtxOffset, 0);
                }
            }
            vtxOffset += cmdList->VtxBuffer.Size;
            idxOffset += cmdList->IdxBuffer.Size;
        }
    }

    void EditorLayer::destroy(const GpuContext& ctx)
    {
        m_pipeline.destroy(ctx);
        m_fontTexture.destroy(ctx);
        m_vertexBuffer.destroy(ctx);
        m_indexBuffer.destroy(ctx);

        if (m_sampler) vkDestroySampler(ctx.device, m_sampler, nullptr);
        if (m_descriptorPool) vkDestroyDescriptorPool(ctx.device, m_descriptorPool, nullptr);
        if (m_textureLayout) vkDestroyDescriptorSetLayout(ctx.device, m_textureLayout, nullptr);
        if (m_samplerLayout) vkDestroyDescriptorSetLayout(ctx.device, m_samplerLayout, nullptr);

        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void EditorLayer::createPipeline(const GpuContext& ctx, VkFormat colorFormat)
    {
        // Define descriptor set layouts
        VkDescriptorSetLayoutBinding textureBinding{};
        textureBinding.binding = 0;
        textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        textureBinding.descriptorCount = 1;
        textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo textureLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        textureLayoutInfo.bindingCount = 1;
        textureLayoutInfo.pBindings = &textureBinding;
        VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &textureLayoutInfo, nullptr, &m_textureLayout));

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo samplerLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        samplerLayoutInfo.bindingCount = 1;
        samplerLayoutInfo.pBindings = &samplerBinding;
        VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &samplerLayoutInfo, nullptr, &m_samplerLayout));

        PipelineConfig config{};
        // We expect shaders to be pre-compiled to .spv
        config.vertexShader = Pipeline::loadShader(ctx.device, "../extern/imgui/glsl_shader.vert.spv");
        config.fragmentShader = Pipeline::loadShader(ctx.device, "../extern/imgui/glsl_shader.frag.spv");
        config.colorFormat = colorFormat;
        config.depthTest = false;
        config.depthWrite = false;
        config.blending = true;
        config.cullMode = VK_CULL_MODE_NONE;

        m_pipeline.create(ctx, config, { m_textureLayout, m_samplerLayout });
    }

    void EditorLayer::createFontAtlas(const GpuContext& ctx)
    {
        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        m_fontTexture.create(ctx, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        m_fontTexture.uploadData(ctx, pixels, width * height * 4);

        // Create sampler
        VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        VK_CHECK(vkCreateSampler(ctx.device, &samplerInfo, nullptr, &m_sampler));

        // Descriptor pool and sets
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1 },
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1 }
        };
        VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.maxSets = 2;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;
        VK_CHECK(vkCreateDescriptorPool(ctx.device, &poolInfo, nullptr, &m_descriptorPool));

        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_textureLayout;
        VK_CHECK(vkAllocateDescriptorSets(ctx.device, &allocInfo, &m_fontDescriptorSet));

        allocInfo.pSetLayouts = &m_samplerLayout;
        VK_CHECK(vkAllocateDescriptorSets(ctx.device, &allocInfo, &m_samplerDescriptorSet));

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_fontTexture.view;
        
        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = m_fontDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writes[0].descriptorCount = 1;
        writes[0].pImageInfo = &imageInfo;

        VkDescriptorImageInfo samplerInfoWrite{};
        samplerInfoWrite.sampler = m_sampler;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = m_samplerDescriptorSet;
        writes[1].dstBinding = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &samplerInfoWrite;

        vkUpdateDescriptorSets(ctx.device, 2, writes, 0, nullptr);
    }
}
