#include "VulkanCommandBuffer.h"
#include "VulkanFramebuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanPipelineState.h"

VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, uint32 queueFamilyIndex, const CommandBufferParams& params)
	: m_Device(device),
	m_CommandPool(VK_NULL_HANDLE),
	m_CommandBuffer(VK_NULL_HANDLE)
{
	Init(queueFamilyIndex, params);
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;

		std::cout << "Destroyed CommandPool" << std::endl;
	}
}

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = flags; 
	beginInfo.pInheritanceInfo = nullptr;

	VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
	if (result != VK_SUCCESS) 
	{
		std::cout << "vkBeginCommandBuffer failed" << std::endl;
	}
}

void VulkanCommandBuffer::BeginRenderPass(VulkanRenderPass* pRenderPass, VulkanFramebuffer* pFramebuffer, VkClearValue* pClearValues, uint32 clearValuesCount)
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

void VulkanCommandBuffer::BindGraphicsPipelineState(VulkanGraphicsPipelineState* pPipelineState)
{
	vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelineState->GetPipeline());
}

void VulkanCommandBuffer::DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
{
	vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::EndRenderPass()
{
	vkCmdEndRenderPass(m_CommandBuffer);
}

void VulkanCommandBuffer::End()
{
	VkResult result = vkEndCommandBuffer(m_CommandBuffer);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkEndCommandBuffer failed" << std::endl;
	}
}

void VulkanCommandBuffer::Reset(VkCommandPoolResetFlags flags)
{
	//Avoid using the VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT since we can reuse the memory
	vkResetCommandPool(m_Device, m_CommandPool, flags);
}

void VulkanCommandBuffer::Init(uint32 queueFamilyIndex, const CommandBufferParams& params)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.flags = 0; 
	poolInfo.queueFamilyIndex = queueFamilyIndex;

	VkResult result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool);
	if (result != VK_SUCCESS) 
	{
		std::cout << "vkCreateCommandPool failed" << std::endl;
		return;
	}
	else
	{
		std::cout << "Created commandpool" << std::endl;
	}

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType		  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.pNext		  = nullptr;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level		  = params.Level;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer) != VK_SUCCESS) 
	{
		std::cout << "vkAllocateCommandBuffers failed" << std::endl;
	}
	else
	{
		std::cout << "Allocated CommandBuffer" << std::endl;
	}
}
