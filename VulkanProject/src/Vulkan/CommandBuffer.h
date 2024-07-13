#pragma once
#include "Device.h"
#include "Buffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineState.h"
#include "DescriptorSet.h"
#include "Query.h"
#include "PipelineLayout.h"
#include <vulkan/vulkan.h>

struct FCommandBufferParams
{
    VkCommandBufferLevel Level;
    ECommandQueueType    QueueType;
};

class FCommandBuffer : public FDeviceChild
{
public:
    static FCommandBuffer* Create(class FDevice* pDevice, const FCommandBufferParams& Params);

    FCommandBuffer(FDevice* pDevice);
    ~FCommandBuffer();

    void BindBindlessDescriptors(FPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint);
    
    void Begin(VkCommandBufferUsageFlags Flags = 0)
    {
        VkCommandBufferBeginInfo BeginInfo = {};
        BeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        BeginInfo.pNext            = nullptr;
        BeginInfo.flags            = Flags;
        BeginInfo.pInheritanceInfo = nullptr;

        VkResult Result = vkBeginCommandBuffer(m_CommandBuffer, &BeginInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "vkBeginCommandBuffer failed. Error: " << Result << std::endl;
        }
    }

    void BeginRenderPass(FRenderPass* pRenderPass, FFramebuffer* pFramebuffer, const VkClearValue* pClearValues, uint32_t ClearValuesCount)
    {
        assert(pRenderPass != nullptr);
        assert(pFramebuffer != nullptr);
        
        VkRenderPassBeginInfo RenderPassInfo = {};
        RenderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassInfo.pNext             = nullptr;
        RenderPassInfo.renderPass        = pRenderPass->GetRenderPass();
        RenderPassInfo.framebuffer       = pFramebuffer->GetFramebuffer();
        RenderPassInfo.renderArea.offset = { 0, 0 };
        RenderPassInfo.renderArea.extent = pFramebuffer->GetExtent();
        RenderPassInfo.pClearValues      = pClearValues;
        RenderPassInfo.clearValueCount   = ClearValuesCount;
        
        vkCmdBeginRenderPass(m_CommandBuffer, &RenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void ClearColorImage(VkImage Image, VkImageLayout ImageLayout, const VkClearColorValue* pColor, uint32_t RangeCount, const VkImageSubresourceRange* pRanges)
    {
        vkCmdClearColorImage(m_CommandBuffer, Image, ImageLayout, pColor, RangeCount, pRanges);
    }

    void SetViewport(const VkViewport& Viewport)
    {
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &Viewport);
    }

    void SetScissorRect(const VkRect2D& Scissor)
    {
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &Scissor);
    }
    
    void WriteTimestamp(FQuery* pQuery, VkPipelineStageFlagBits PipelineStage, uint32_t QueryIndex)
    {
        assert(pQuery != nullptr);
        vkCmdWriteTimestamp(m_CommandBuffer, PipelineStage, pQuery->GetQueryPool(), QueryIndex);
    }

    void BindGraphicsPipelineState(FGraphicsPipeline* pPipelineState)
    {
        assert(pPipelineState != nullptr);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->GetPipeline());
    }
    
    void BindComputePipelineState(FComputePipeline* pPipelineState)
    {
        assert(pPipelineState != nullptr);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineState->GetPipeline());
    }
    
    void BindGraphicsDescriptorSet(FPipelineLayout* pPipelineLayout, FDescriptorSet* pDescriptorSet, uint32_t DescriptorSetIndex)
    {
        assert(pDescriptorSet != nullptr);
        assert(pPipelineLayout != nullptr);
        VkDescriptorSet DescriptorSet = pDescriptorSet->GetDescriptorSet();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineLayout->GetPipelineLayout(), DescriptorSetIndex, 1, &DescriptorSet, 0, nullptr);
    }
    
    void BindComputeDescriptorSet(FPipelineLayout* pPipelineLayout, FDescriptorSet* pDescriptorSet, uint32_t DescriptorSetIndex)
    {
        assert(pDescriptorSet != nullptr);
        assert(pPipelineLayout != nullptr);
        VkDescriptorSet DescriptorSet = pDescriptorSet->GetDescriptorSet();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineLayout->GetPipelineLayout(), DescriptorSetIndex, 1, &DescriptorSet, 0, nullptr);
    }

    void BindVertexBuffer(FBuffer* pBuffer, VkDeviceSize Offset, uint32_t Slot)
    {
        assert(pBuffer != nullptr);

        VkBuffer Buffer[] = { pBuffer->GetBuffer() };
        VkDeviceSize Offsets[] = { Offset };
        vkCmdBindVertexBuffers(m_CommandBuffer, Slot, 1, Buffer, Offsets);
    }

    void BindIndexBuffer(FBuffer* pBuffer, VkDeviceSize Offset, VkIndexType IndexType)
    {
        assert(pBuffer != nullptr);
        vkCmdBindIndexBuffer(m_CommandBuffer, pBuffer->GetBuffer(), Offset, IndexType);
    }
    
    void PushConstants(FPipelineLayout* pPipelineLayout, VkShaderStageFlags StageFlags, uint32_t Offset, uint32_t Size, const void* pData)
    {
        assert(pPipelineLayout != nullptr);
        vkCmdPushConstants(m_CommandBuffer, pPipelineLayout->GetPipelineLayout(), StageFlags, Offset, Size, pData);
    }
    
    void TransitionImage(VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags AspectMask);
    
    void UpdateBuffer(FBuffer* pBuffer, VkDeviceSize DstOffset, VkDeviceSize DataSize, const void* pData)
    {
        assert(pBuffer != nullptr);
        assert(DataSize < 65536); // Ensure that we are within the allowed Size
        vkCmdUpdateBuffer(m_CommandBuffer, pBuffer->GetBuffer(), DstOffset, DataSize, pData);
    }
    
    void CopyBuffer(VkBuffer SrcBuffer, VkBuffer DstBuffer, uint32_t RegionCount, const VkBufferCopy* pRegions)
    {
        vkCmdCopyBuffer(m_CommandBuffer, SrcBuffer, DstBuffer, RegionCount, pRegions);
    }

    void CopyBufferToImage(VkBuffer SrcBuffer, VkImage DstImage, VkImageLayout DstImageLayout, uint32_t RegionCount, const VkBufferImageCopy* pRegions)
    {
        vkCmdCopyBufferToImage(m_CommandBuffer, SrcBuffer, DstImage, DstImageLayout, RegionCount, pRegions);
    }

    void DrawInstanced(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance)
    {
        vkCmdDraw(m_CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
    }

    void DrawIndexInstanced(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex, uint32_t VertexOffset, uint32_t FirstInstance)
    {
        vkCmdDrawIndexed(m_CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
    }
    
    void Dispatch(uint32_t ThreadGroupsX, uint32_t ThreadGroupsY, uint32_t ThreadGroupsZ)
    {
        vkCmdDispatch(m_CommandBuffer, ThreadGroupsX, ThreadGroupsY, ThreadGroupsZ);
    }

    void EndRenderPass()
    {
        vkCmdEndRenderPass(m_CommandBuffer);
    }

    void End()
    {
        VkResult Result = vkEndCommandBuffer(m_CommandBuffer);
        if (Result != VK_SUCCESS)
        {
            std::cout << "vkEndCommandBuffer failed. Error: " << Result << std::endl;
        }
    }
    
    bool IsFinishedOnGPU() const
    {
        VkResult Result = vkGetFenceStatus(GetDevice()->GetDevice(), m_Fence);
        return Result == VK_NOT_READY;
    }
    
    void WaitForAndResetFences()
    {
        vkWaitForFences(GetDevice()->GetDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX);
        vkResetFences(GetDevice()->GetDevice(), 1, &m_Fence);
    }

    void Reset(VkCommandPoolResetFlags Flags = 0)
    {
        // Wait for GPU to finish with this CommandBuffer and then reset it
        WaitForAndResetFences();
        
        // Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
        vkResetCommandPool(GetDevice()->GetDevice(), m_CommandPool, Flags);
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
    VkFence         m_Fence;
    VkCommandPool   m_CommandPool;
    VkCommandBuffer m_CommandBuffer;
};
