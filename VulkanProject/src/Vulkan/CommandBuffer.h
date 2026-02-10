#pragma once
#include "Device.h"
#include "Buffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineState.h"
#include "DescriptorSet.h"
#include "Query.h"
#include "PipelineLayout.h"
#include "Extensions.h"

struct SCommandBufferParams
{
    VkCommandBufferLevel Level;
    ECommandQueueType    QueueType;
};

class CCommandBuffer : public CDeviceChild
{
public:
    static CCommandBuffer* Create(class CDevice* pDevice, const SCommandBufferParams& Params);

    CCommandBuffer(CDevice* pDevice);
    ~CCommandBuffer();

    void BindBindlessDescriptors(CPipelineLayout* pPipelineLayout, VkPipelineBindPoint BindPoint);
    void TransitionImage(VkImage Image, VkImageLayout OldLayout, VkImageLayout NewLayout, VkImageAspectFlags AspectMask);

    void SetDebugName(const char* DebugName);

    void PipelineBarrier(VkPipelineStageFlags SrcStageMask, VkPipelineStageFlags DstStageMask, VkDependencyFlags DependencyFlags, uint32_t MemoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t BufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t ImageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
    {
        vkCmdPipelineBarrier(m_CommandBuffer, SrcStageMask, DstStageMask, DependencyFlags, MemoryBarrierCount, pMemoryBarriers, BufferMemoryBarrierCount, pBufferMemoryBarriers, ImageMemoryBarrierCount, pImageMemoryBarriers);
        m_NumCommands++;
    }

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
            LOG("vkBeginCommandBuffer failed. Error: %d\n", Result);
        }
    }

    void BeginRenderPass(CRenderPass* pRenderPass, CFramebuffer* pFramebuffer, const VkClearValue* pClearValues, uint32_t ClearValuesCount)
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
        m_NumCommands++;
    }

    void ClearColorImage(VkImage Image, VkImageLayout ImageLayout, const VkClearColorValue* pColor, uint32_t RangeCount, const VkImageSubresourceRange* pRanges)
    {
        vkCmdClearColorImage(m_CommandBuffer, Image, ImageLayout, pColor, RangeCount, pRanges);
        m_NumCommands++;
    }

    void SetViewport(const VkViewport& Viewport)
    {
        vkCmdSetViewport(m_CommandBuffer, 0, 1, &Viewport);
        m_NumCommands++;
    }

    void SetScissorRect(const VkRect2D& Scissor)
    {
        vkCmdSetScissor(m_CommandBuffer, 0, 1, &Scissor);
        m_NumCommands++;
    }

    void WriteTimestamp(CQuery* pQuery, VkPipelineStageFlagBits PipelineStage, uint32_t QueryIndex)
    {
        assert(pQuery != nullptr);
        vkCmdWriteTimestamp(m_CommandBuffer, PipelineStage, pQuery->GetQueryPool(), QueryIndex);
        m_NumCommands++;
    }

    void BuildAccelerationStructures(uint32_t InfoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
    {
        assert(pInfos != nullptr);
        assert(ppBuildRangeInfos != nullptr);
        Extensions::vkCmdBuildAccelerationStructuresKHR(m_CommandBuffer, InfoCount, pInfos, ppBuildRangeInfos);
        m_NumCommands++;
    }

    void BindGraphicsPipelineState(CGraphicsPipeline* pPipelineState)
    {
        assert(pPipelineState != nullptr);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->GetPipeline());
        m_NumCommands++;
    }

    void BindComputePipelineState(CComputePipeline* pPipelineState)
    {
        assert(pPipelineState != nullptr);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineState->GetPipeline());
        m_NumCommands++;
    }

    void BindRayTracingPipelineState(CRayTracingPipeline* pPipelineState)
    {
        assert(pPipelineState != nullptr);
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pPipelineState->GetPipeline());
        m_NumCommands++;
    }

    void BindGraphicsDescriptorSet(CPipelineLayout* pPipelineLayout, CDescriptorSet* pDescriptorSet, uint32_t DescriptorSetIndex)
    {
        assert(pDescriptorSet != nullptr);
        assert(pPipelineLayout != nullptr);
        VkDescriptorSet DescriptorSet = pDescriptorSet->GetDescriptorSet();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineLayout->GetPipelineLayout(), DescriptorSetIndex, 1, &DescriptorSet, 0, nullptr);
        m_NumCommands++;
    }

    void BindComputeDescriptorSet(CPipelineLayout* pPipelineLayout, CDescriptorSet* pDescriptorSet, uint32_t DescriptorSetIndex)
    {
        assert(pDescriptorSet != nullptr);
        assert(pPipelineLayout != nullptr);
        VkDescriptorSet DescriptorSet = pDescriptorSet->GetDescriptorSet();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineLayout->GetPipelineLayout(), DescriptorSetIndex, 1, &DescriptorSet, 0, nullptr);
        m_NumCommands++;
    }

    void BindRayTracingDescriptorSet(CPipelineLayout* pPipelineLayout, CDescriptorSet* pDescriptorSet, uint32_t DescriptorSetIndex)
    {
        assert(pDescriptorSet != nullptr);
        assert(pPipelineLayout != nullptr);
        VkDescriptorSet DescriptorSet = pDescriptorSet->GetDescriptorSet();
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pPipelineLayout->GetPipelineLayout(), DescriptorSetIndex, 1, &DescriptorSet, 0, nullptr);
        m_NumCommands++;
    }

    void BindVertexBuffer(CBuffer* pBuffer, VkDeviceSize Offset, uint32_t Slot)
    {
        assert(pBuffer != nullptr);

        VkBuffer Buffer[] = { pBuffer->GetBuffer() };
        VkDeviceSize Offsets[] = { Offset };
        vkCmdBindVertexBuffers(m_CommandBuffer, Slot, 1, Buffer, Offsets);
        m_NumCommands++;
    }

    void BindIndexBuffer(CBuffer* pBuffer, VkDeviceSize Offset, VkIndexType IndexType)
    {
        assert(pBuffer != nullptr);
        vkCmdBindIndexBuffer(m_CommandBuffer, pBuffer->GetBuffer(), Offset, IndexType);
        m_NumCommands++;
    }

    void PushConstants(CPipelineLayout* pPipelineLayout, VkShaderStageFlags StageFlags, uint32_t Offset, uint32_t Size, const void* pData)
    {
        assert(pPipelineLayout != nullptr);
        vkCmdPushConstants(m_CommandBuffer, pPipelineLayout->GetPipelineLayout(), StageFlags, Offset, Size, pData);
        m_NumCommands++;
    }

    void UpdateBuffer(CBuffer* pBuffer, VkDeviceSize DstOffset, VkDeviceSize DataSize, const void* pData)
    {
        assert(pBuffer != nullptr);
        assert(DataSize < 65536); // Ensure that we are within the allowed Size
        vkCmdUpdateBuffer(m_CommandBuffer, pBuffer->GetBuffer(), DstOffset, DataSize, pData);
        m_NumCommands++;
    }

    void FillBuffer(CBuffer* pBuffer, VkDeviceSize DstOffset, VkDeviceSize Size, uint32_t Data)
    {
        assert(pBuffer != nullptr);
        vkCmdFillBuffer(m_CommandBuffer, pBuffer->GetBuffer(), DstOffset, Size, Data);
        m_NumCommands++;
    }

    void CopyBuffer(VkBuffer SrcBuffer, VkBuffer DstBuffer, uint32_t RegionCount, const VkBufferCopy* pRegions)
    {
        vkCmdCopyBuffer(m_CommandBuffer, SrcBuffer, DstBuffer, RegionCount, pRegions);
        m_NumCommands++;
    }

    void CopyBufferToImage(VkBuffer SrcBuffer, VkImage DstImage, VkImageLayout DstImageLayout, uint32_t RegionCount, const VkBufferImageCopy* pRegions)
    {
        vkCmdCopyBufferToImage(m_CommandBuffer, SrcBuffer, DstImage, DstImageLayout, RegionCount, pRegions);
        m_NumCommands++;
    }

    void DrawInstanced(uint32_t VertexCount, uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance)
    {
        vkCmdDraw(m_CommandBuffer, VertexCount, InstanceCount, FirstVertex, FirstInstance);
        m_NumCommands++;
    }

    void DrawIndexInstanced(uint32_t IndexCount, uint32_t InstanceCount, uint32_t FirstIndex, uint32_t VertexOffset, uint32_t FirstInstance)
    {
        vkCmdDrawIndexed(m_CommandBuffer, IndexCount, InstanceCount, FirstIndex, VertexOffset, FirstInstance);
        m_NumCommands++;
    }

    void Dispatch(uint32_t ThreadGroupsX, uint32_t ThreadGroupsY, uint32_t ThreadGroupsZ)
    {
        vkCmdDispatch(m_CommandBuffer, ThreadGroupsX, ThreadGroupsY, ThreadGroupsZ);
        m_NumCommands++;
    }

    void TraceRays(CRayTracingPipeline* pPipelineState, uint32_t Width, uint32_t Height, uint32_t Depth)
    {
        assert(pPipelineState != nullptr);
        Extensions::vkCmdTraceRaysKHR(m_CommandBuffer, pPipelineState->GetRayGenSBT(), pPipelineState->GetRayMissSBT(), pPipelineState->GetRayHitSBT(), pPipelineState->GetRayCallableSBT(), Width, Height, Depth);
        m_NumCommands++;
    }

    void EndRenderPass()
    {
        vkCmdEndRenderPass(m_CommandBuffer);
        m_NumCommands++;
    }

    void End()
    {
        VkResult Result = vkEndCommandBuffer(m_CommandBuffer);
        if (Result != VK_SUCCESS)
        {
            LOG("vkEndCommandBuffer failed. Error: %d\n", Result);
        }
    }

    bool IsFinishedOnGPU() const
    {
        VkResult Result = vkGetFenceStatus(GetDevice()->GetDevice(), m_Fence);
        return Result == VK_NOT_READY;
    }

    void WaitForAndResetFences()
    {
        VkResult Result = vkWaitForFences(GetDevice()->GetDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX);
        if (Result != VK_SUCCESS)
        {
            DEBUG_BREAK();
        }

        Result = vkResetFences(GetDevice()->GetDevice(), 1, &m_Fence);
        if (Result != VK_SUCCESS)
        {
            DEBUG_BREAK();
        }
    }

    void Reset(VkCommandPoolResetFlags Flags = 0)
    {
        // Wait for GPU to finish with this CommandBuffer and then reset it
        WaitForAndResetFences();
        
        // Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
        VkResult Result = vkResetCommandPool(GetDevice()->GetDevice(), m_CommandPool, Flags);
        if (Result != VK_SUCCESS)
        {
            DEBUG_BREAK();
        }

        // Reset CommandCount
        m_NumCommands = 0;
    }

    VkFence GetFence() const
    {
        return m_Fence;
    }

    VkCommandBuffer GetCommandBuffer() const
    {
        return m_CommandBuffer;
    }

    uint32_t GetNumCommands() const
    {
        return m_NumCommands;
    }

private:
    VkFence         m_Fence;
    VkCommandPool   m_CommandPool;
    VkCommandBuffer m_CommandBuffer;
    uint32_t        m_NumCommands;
};
