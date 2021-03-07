#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

#include "Buffer.h"
#include "RenderPass.h"
#include "Framebuffer.h"
#include "PipelineState.h"
#include "DescriptorSet.h"

enum class ECommandQueueType
{
	Graphics = 1,
	Compute  = 2,
	Transfer = 3,
};

struct CommandBufferParams
{
	VkCommandBufferLevel Level;
	ECommandQueueType QueueType;
};

class CommandBuffer
{
public:
	CommandBuffer(VkDevice device);
	~CommandBuffer();

	inline void Begin(VkCommandBufferUsageFlags flags = 0)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = flags;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkBeginCommandBuffer failed. Error: " << result << std::endl;
		}
	}

	inline void BeginRenderPass(RenderPass* pRenderPass, Framebuffer* pFramebuffer, VkClearValue* pClearValues, uint32_t clearValuesCount)
	{
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType		= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.pNext		= nullptr;
		renderPassInfo.renderPass	= pRenderPass->GetRenderPass();
		renderPassInfo.framebuffer	= pFramebuffer->GetFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = pFramebuffer->GetExtent();
		renderPassInfo.pClearValues		 = pClearValues;
		renderPassInfo.clearValueCount	 = clearValuesCount;
		
		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	inline void SetViewport(const VkViewport& viewport)
	{
		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
	}

	inline void SetScissorRect(const VkRect2D& scissor)
	{
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
	}

	inline void BindGraphicsPipelineState(GraphicsPipeline* pPipelineState)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->GetPipeline());
	}
	
	inline void BindComputePipelineState(ComputePipeline* pPipelineState)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipelineState->GetPipeline());
	}
	
	inline void BindGraphicsDescriptorSet(GraphicsPipeline* pPipeline, DescriptorSet* pDescriptorSet)
	{
		VkDescriptorSet descriptorSet = pDescriptorSet->GetDescriptorSet();
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipeline->GetPiplineLayout(), 0, 1, &descriptorSet, 0, nullptr);
	}
	
	inline void BindComputeDescriptorSet(ComputePipeline* pPipeline, DescriptorSet* pDescriptorSet)
	{
		VkDescriptorSet descriptorSet = pDescriptorSet->GetDescriptorSet();
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pPipeline->GetPiplineLayout(), 0, 1, &descriptorSet, 0, nullptr);
	}

	inline void BindVertexBuffer(Buffer* pBuffer, VkDeviceSize offset, uint32_t slot)
	{
		assert(pBuffer);

		VkBuffer buffer[] = { pBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { offset };
		vkCmdBindVertexBuffers(m_CommandBuffer, slot, 1, buffer, offsets);
	}

	inline void BindIndexBuffer(Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
	{
		assert(pBuffer != nullptr);
		vkCmdBindIndexBuffer(m_CommandBuffer, pBuffer->GetBuffer(), offset, indexType);
	}
	
	inline void TransitionImage(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType 							= VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout 						= oldLayout;
		barrier.newLayout 						= newLayout;
		barrier.srcQueueFamilyIndex 			= VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex 			= VK_QUEUE_FAMILY_IGNORED;
		barrier.image 							= image;
		barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel   = 0;
		barrier.subresourceRange.levelCount     = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount     = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = 0;

			sourceStage      = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else
		{
			std::cout << "unsupported layout transition!" << std::endl;
			return;
		}

		vkCmdPipelineBarrier(m_CommandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}
	
	inline void UpdateBuffer(Buffer* pBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
	{
		vkCmdUpdateBuffer(m_CommandBuffer, pBuffer->GetBuffer(), dstOffset, dataSize, pData);
	}

	inline void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	inline void DrawIndexInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}
	
	inline void Dispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ)
	{
		vkCmdDispatch(m_CommandBuffer, threadGroupsX, threadGroupsY, threadGroupsZ);
	}

	inline void EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}

	inline void End()
	{
		VkResult result = vkEndCommandBuffer(m_CommandBuffer);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkEndCommandBuffer failed. Error: " << result << std::endl;
		}
	}

	inline void Reset(VkCommandPoolResetFlags flags = 0)
	{
		// Wait for GPU to finish with this commandbuffer and then reset it
		vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_Fence);

		// Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
		vkResetCommandPool(m_Device, m_CommandPool, flags);
	}

	inline VkFence GetFence() const
	{
		return m_Fence;
	}
	
	inline VkCommandBuffer GetCommandBuffer() const
	{
		return m_CommandBuffer;
	}
	
	static CommandBuffer* Create(class VulkanContext* pContext, const CommandBufferParams& params);
	
private:
	VkDevice m_Device;
	VkFence  m_Fence;
	VkCommandPool   m_CommandPool;
	VkCommandBuffer m_CommandBuffer;
};
