#include "VulkanBuffer.h"
#include "VulkanHelper.h"

void VulkanBuffer::Init(const BufferParams& params)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.flags = 0;
	bufferInfo.size	 = params.SizeInBytes;
	bufferInfo.usage = params.Usage;
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices	 = nullptr;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(m_Device, &bufferInfo, nullptr, &m_Buffer);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkCreateBuffer failed. Error: " << result << std::endl;
	}
	else
	{
		m_SizeInBytes = params.SizeInBytes;
		std::cout << "Created Buffer" << std::endl;
	}

	VkMemoryRequirements memRequirements = {};
	vkGetBufferMemoryRequirements(m_Device, m_Buffer, &memRequirements);
	
	if (m_pAllocator)
	{
		if (m_pAllocator->Allocate(m_Allocation, memRequirements, params.MemoryProperties))
		{
			vkBindBufferMemory(m_Device, m_Buffer, m_Allocation.DeviceMemory, m_Allocation.DeviceMemoryOffset);
		}
		else
		{
			std::cout << "VulkanDeviceAllocator::Allocate failed" << std::endl;
		}
	}
	else
	{
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		result = vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_DeviceMemory);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkAllocateMemory failed. Error: " << result << std::endl;
		}
		else
		{
			vkBindBufferMemory(m_Device, m_Buffer, m_DeviceMemory, 0);
			std::cout << "Allocated " << memRequirements.size << " bytes" << std::endl;
		}
	}
}
