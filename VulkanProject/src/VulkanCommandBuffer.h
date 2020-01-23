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
    void SetViewport(const VkViewport& viewport);
    void SetScissorRect(const VkRect2D& scissor);
    void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance);
	void EndRenderPass();
	void End();
	
	void Reset(VkCommandPoolResetFlags flags = 0);

	VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
private:
	void Init(uint32 queueFamilyIndex, const CommandBufferParams& params);
private:
	VkDevice m_Device;
	VkCommandPool m_CommandPool;
	VkCommandBuffer m_CommandBuffer;
};
