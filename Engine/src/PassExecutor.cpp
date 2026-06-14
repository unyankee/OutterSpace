#include "PassExecutor.h"
#include "Pipeline.h"
#include "PipelineManager.h"
#include "ResourceManager.h"

namespace ToyEngine
{
    void PassExecutor::execute(VkCommandBuffer cmd, const RenderBatch& batch, PassContext& ctx)
    {
        std::vector<VkRenderingAttachmentInfo> colorInfos;
        for (const auto& attachment : batch.colorAttachments)
        {
            RenderTarget* rt = ctx.resourceManager.getRenderTarget(attachment.handle);
            VkRenderingAttachmentInfo info = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            info.imageView = rt->m_view;
            info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            info.loadOp = attachment.loadOp;
            info.storeOp = attachment.storeOp;
            info.clearValue = attachment.clearValue;
            colorInfos.push_back(info);
        }

        VkRenderingAttachmentInfo depthInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        if (batch.useDepth)
        {
            RenderTarget* rt = ctx.resourceManager.getRenderTarget(batch.depthAttachment.handle);
            depthInfo.imageView = rt->m_view;
            depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthInfo.loadOp = batch.depthAttachment.loadOp;
            depthInfo.storeOp = batch.depthAttachment.storeOp;
            depthInfo.clearValue = batch.depthAttachment.clearValue;
        }

        uint32_t width = 0;
        uint32_t height = 0;
        if (!colorInfos.empty())
        {
            RenderTarget* rt = ctx.resourceManager.getRenderTarget(batch.colorAttachments[0].handle);
            width = rt->m_width;
            height = rt->m_height;
        }

        VkRenderingInfo renderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        renderingInfo.renderArea = {{0, 0}, {width, height}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = (uint32_t)colorInfos.size();
        renderingInfo.pColorAttachments = colorInfos.data();
        if (batch.useDepth)
        {
            renderingInfo.pDepthAttachment = &depthInfo;
        }

        vkCmdBeginRendering(cmd, &renderingInfo);

        for (const auto& pass : batch.passes)
        {
            if (pass.pipeline.isValid())
            {
                Pipeline* pipeline = ctx.resourceManager.getPipeline(pass.pipeline);
                pipeline->bind(cmd);

                VkDescriptorSet globalSet = ctx.pipelineManager.getGlobalDescriptorSet();
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getLayout(), 0, 1, &globalSet,
                                        0, nullptr);
            }

            if (pass.execute)
            {
                pass.execute(cmd, pass, ctx);
            }
        }

        vkCmdEndRendering(cmd);
    }
}
