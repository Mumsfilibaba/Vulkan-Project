#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

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
	DECL_NO_COPY(VulkanCommandBuffer);

	VulkanCommandBuffer(VkDevice device, uint32 queueFamilyIndex, const CommandBufferParams& params);
	~VulkanCommandBuffer();

	void Begin(VkCommandBufferUsageFlags flags = 0);
	void BeginRenderPass(VulkanRenderPass* pRenderPass, VulkanFramebuffer* pFramebuffer, VkClearValue* pClearValues, uint32 clearValuesCount);
	
	void BindGraphicsPipelineState(VulkanGraphicsPipelineState* pPipelineState);
	void BindVertexBuffer(VulkanBuffer* pBuffer, VkDeviceSize offset, uint32 slot);
	void BindIndexBuffer(VulkanBuffer* pBuffer, VkDeviceSize offset, VkIndexType indexType);
    
	void SetViewport(const VkViewport& viewport);
    void SetScissorRect(const VkRect2D& scissor);
	
    void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);
	void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance);
	
	void EndRenderPass();
	void End();
	
	void Reset(VkCommandPoolResetFlags flags = 0);

	VkFence GetFence() const { return m_Fence; }
	VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
private:
	void Init(uint32 queueFamilyIndex, const CommandBufferParams& params);
private:
	VkDevice m_Device;
	VkFence m_Fence;
	VkCommandPool m_CommandPool;
	VkCommandBuffer m_CommandBuffer;
};
