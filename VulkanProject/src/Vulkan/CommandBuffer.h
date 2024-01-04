#pragma once
#include "Core.h"
#include "Buffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineState.h"
#include "DescriptorSet.h"
#include "Query.h"
#include "PipelineLayout.h"
#include <vulkan/vulkan.h>

enum class ECommandQueueType
{
    Graphics = 1,
    Compute  = 2,
    Transfer = 3,
};

struct CommandBufferParams
{
    VkCommandBufferLevel Level;
    ECommandQueueType    QueueType;
};

class CommandBuffer
{
public:
    static CommandBuffer* Create(class Device* pDevice, const CommandBufferParams& params);

    CommandBuffer(VkDevice device);
    ~CommandBuffer();

    void Begin(VkCommandBufferUsageFlags flags = 0)
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext            = nullptr;
        beginInfo.flags            = flags;
        beginInfo.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
        if (result != VK_SUCCESS)
        {
            std::cout << "vkBeginCommandBuffer failed. Error: " << result << std::endl;
        }
    }

    void BeginRenderPass(RenderPass* pRenderPass, Framebuffer* pFramebuffer, const VkClearValue* pClearValues, uint32_t clearValuesCount)
    {
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.pNext             = nullptr;
        renderPassInfo.renderPass        = pRenderPass->GetRenderPass();
        renderPassInfo.framebuffer       = pFramebuffer->GetFramebuffer();
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = pFramebuffer->GetExtent();
        renderPassInfo.pClearValues      = pClearValues;
        renderPassInfo.clearValueCount   = clearValuesCount;
        
        vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void SetViewport(const VkViewport& viewport)
    {
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
    }

    void SetScissorRect(const VkRect2D& scissor)
    {
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
    }
    
    void WriteTimestamp(Query* pQuery, VkPipelineStageFlagBits pipelineStage, uint32_t queryIndex)
    {
        vkCmdWriteTimestamp(m_CommandBuffer, pipelineStage, pQuery->GetQueryPool(), queryIndex);
    }

    void BindGraphicsPipelineState(GraphicsPipeline* pPipelineState)
    {
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->GetPipeline());
    }
    
    void BindComputePipelineState(ComputePipeline* pPipelineState)
    {
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineState->GetPipeline());
    }
    
    void BindGraphicsDescriptorSet(PipelineLayout* pPipelineLayout, DescriptorSet* pDescriptorSet)
    {
        VkDescriptorSet descriptorSet = pDescriptorSet->GetDescriptorSet();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineLayout->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
    }
    
    void BindComputeDescriptorSet(PipelineLayout* pPipelineLayout, DescriptorSet* pDescriptorSet)
    {
        VkDescriptorSet descriptorSet = pDescriptorSet->GetDescriptorSet();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineLayout->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);
    }

    void BindVertexBuffer(Buffer* pBuffer, VkDeviceSize offset, uint32_t slot)
    {
        assert(pBuffer);

        VkBuffer buffer[] = { pBuffer->GetBuffer() };
        VkDeviceSize offsets[] = { offset };
        vkCmdBindVertexBuffers(m_CommandBuffer, slot, 1, buffer, offsets);
    }

    void BindIndexBuffer(Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
    {
        assert(pBuffer != nullptr);
        vkCmdBindIndexBuffer(m_CommandBuffer, pBuffer->GetBuffer(), offset, indexType);
    }
    
    void PushConstants(PipelineLayout* pPipelineLayout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pData)
    {
        vkCmdPushConstants(m_CommandBuffer, pPipelineLayout->GetPipelineLayout(), stageFlags, offset, size, pData);
    }
    
    void TransitionImage(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
    
    void UpdateBuffer(Buffer* pBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
    {
        vkCmdUpdateBuffer(m_CommandBuffer, pBuffer->GetBuffer(), dstOffset, dataSize, pData);
    }
    
    void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
    {
        vkCmdCopyBufferToImage(m_CommandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }

    void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void DrawIndexInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
    {
        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
    
    void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ)
    {
        vkCmdDispatch(m_CommandBuffer, threadGroupsX, threadGroupsY, threadGroupsZ);
    }

    void EndRenderPass()
    {
        vkCmdEndRenderPass(m_CommandBuffer);
    }

    void End()
    {
        VkResult result = vkEndCommandBuffer(m_CommandBuffer);
        if (result != VK_SUCCESS)
        {
            std::cout << "vkEndCommandBuffer failed. Error: " << result << std::endl;
        }
    }
    
    bool IsFinishedOnGPU() const
    {
        VkResult result = vkGetFenceStatus(m_Device, m_Fence);
        return result == VK_NOT_READY;
    }
    
    void WaitForAndResetFences()
    {
        vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device, 1, &m_Fence);
    }

    void Reset(VkCommandPoolResetFlags flags = 0)
    {
        // Wait for GPU to finish with this commandbuffer and then reset it
        WaitForAndResetFences();
        
        // Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
        vkResetCommandPool(m_Device, m_CommandPool, flags);
    }

    VkFence GetFence() const
    {
        return m_Fence;
    }
    
    VkCommandBuffer GetCommandBuffer() const
    {
        return m_CommandBuffer;
    }
    
private:
    VkDevice        m_Device;
    VkFence         m_Fence;
    VkCommandPool   m_CommandPool;
    VkCommandBuffer m_CommandBuffer;
};
