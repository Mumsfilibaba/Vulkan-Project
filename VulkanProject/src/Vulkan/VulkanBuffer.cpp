#include "VulkanBuffer.h"
#include "VulkanHelper.h"

VulkanBuffer::VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferParams& params)
	: m_Device(device),
	m_Buffer(VK_NULL_HANDLE),
	m_Memory(VK_NULL_HANDLE),
	m_SizeInBytes(0)
{
	Init(physicalDevice, params);
}

VulkanBuffer::~VulkanBuffer()
{
	if (m_Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_Device, m_Buffer, nullptr);
		m_Buffer = VK_NULL_HANDLE;
	}

	if (m_Memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_Device, m_Memory, nullptr);
		m_Memory = nullptr;
	}
}

void VulkanBuffer::Init(VkPhysicalDevice physicalDevice, const BufferParams& params)
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

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = nullptr;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	result = vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_Memory);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkAllocateMemory failed. Error: " << result << std::endl;
	}
	else
	{
		vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0);
		std::cout << "Allocated " << m_SizeInBytes << " bytes" << std::endl;
	}
}

void VulkanBuffer::Map(void** ppCPUMem)
{
	VkResult result = vkMapMemory(m_Device, m_Memory, 0, m_SizeInBytes, 0, ppCPUMem);
	if (result != VK_SUCCESS)
	{
		std::cout << "vkMapMemory failed. Error: " << result << std::endl;
	}
}

void VulkanBuffer::Unmap()
{
	vkUnmapMemory(m_Device, m_Memory);
}