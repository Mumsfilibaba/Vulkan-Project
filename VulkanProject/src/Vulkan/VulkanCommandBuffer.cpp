#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipelineState.h"

VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, uint32_t queueFamilyIndex, const CommandBufferParams& params)
	: m_Device(device),
	m_CommandPool(VK_NULL_HANDLE),
	m_CommandBuffer(VK_NULL_HANDLE),
	m_Fence(VK_NULL_HANDLE)
{
	Init(queueFamilyIndex, params);
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	if (m_Fence != VK_NULL_HANDLE)
	{
		vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX);

		vkDestroyFence(m_Device, m_Fence, nullptr);
		m_Fence = VK_NULL_HANDLE;
	}

	if (m_CommandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		m_CommandPool = VK_NULL_HANDLE;

		std::cout << "Destroyed CommandPool" << std::endl;
	}
}

void VulkanCommandBuffer::Init(uint32_t queueFamilyIndex, const CommandBufferParams& params)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.pNext = nullptr;
	poolInfo.flags = 0;
	poolInfo.queueFamilyIndex = queueFamilyIndex;

	VkResult result = vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateCommandPool failed. Error: " << result << std::endl;
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

	result = vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer);
	if (result != VK_SUCCESS) 
	{
		std::cout << "vkAllocateCommandBuffers failed. Error: " << result << std::endl;
	}
	else
	{
		std::cout << "Allocated CommandBuffer" << std::endl;
	}

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.pNext = nullptr;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	result = vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fence);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateFence failed. Error: " << result << std::endl;
	}
	else
	{
		std::cout << "Created Fence for commandbuffer" << std::endl;
	}
}
