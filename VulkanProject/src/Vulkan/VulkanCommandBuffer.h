#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipelineState.h"

enum class ECommandQueueType
{
	COMMAND_QUEUE_TYPE_GRAPHICS	= 1,
	COMMAND_QUEUE_TYPE_COMPUTE	= 2,
	COMMAND_QUEUE_TYPE_TRANSFER	= 3,
};

struct CommandBufferParams
{
	VkCommandBufferLevel Level;
	ECommandQueueType QueueType;
};

class VulkanBuffer;
class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanGraphicsPipelineState;

class VulkanCommandBuffer
{
public:
	VulkanCommandBuffer(VkDevice device, uint32_t queueFamilyIndex, const CommandBufferParams& params);
	~VulkanCommandBuffer();

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

	inline void BeginRenderPass(VulkanRenderPass* pRenderPass, VulkanFramebuffer* pFramebuffer, VkClearValue* pClearValues, uint32_t clearValuesCount)
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

	inline void BindGraphicsPipelineState(VulkanGraphicsPipelineState* pPipelineState)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->GetPipeline());
	}

	inline void BindVertexBuffer(VulkanBuffer* pBuffer, VkDeviceSize offset, uint32_t slot)
	{
		assert(pBuffer);

		VkBuffer buffer[] = { pBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { offset };
		vkCmdBindVertexBuffers(m_CommandBuffer, slot, 1, buffer, offsets);
	}

	inline void BindIndexBuffer(VulkanBuffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
	{
		assert(pBuffer != nullptr);

		vkCmdBindIndexBuffer(m_CommandBuffer, pBuffer->GetBuffer(), offset, indexType);
	}

	inline void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	inline void DrawIndexInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
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
		//Wait for GPU to finish with this commandbuffer and then reset it
		vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_Device, 1, &m_Fence);

		//Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
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
	
private:
	void Init(uint32_t queueFamilyIndex, const CommandBufferParams& params);
	
private:
	VkDevice m_Device;
	VkFence m_Fence;
	VkCommandPool m_CommandPool;
	VkCommandBuffer m_CommandBuffer;
};
