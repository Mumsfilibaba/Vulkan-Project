#include "Buffer.h"
#include "VulkanHelper.h"
#include "VulkanContext.h"

Buffer* Buffer::Create(VulkanContext* pContext, const BufferParams& params, VulkanDeviceAllocator* pAllocator)
{
    Buffer* newBuffer = new Buffer(pContext->GetDevice(), pContext->GetPhysicalDevice(), pAllocator);
    
    VkBufferCreateInfo bufferInfo;
    ZERO_STRUCT(&bufferInfo);
    
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = params.SizeInBytes;
    bufferInfo.usage       = params.Usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(newBuffer->m_Device, &bufferInfo, nullptr, &newBuffer->m_Buffer);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateBuffer failed. Error: " << result << std::endl;
        return nullptr;
    }
    else
    {
        newBuffer->m_SizeInBytes = params.SizeInBytes;
        std::cout << "Created Buffer" << std::endl;
    }

    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(newBuffer->m_Device, newBuffer->m_Buffer, &memRequirements);
    
    if (pAllocator)
    {
        if (pAllocator->Allocate(newBuffer->m_Allocation, memRequirements, params.MemoryProperties))
        {
            vkBindBufferMemory(newBuffer->m_Device, newBuffer->m_Buffer, newBuffer->m_Allocation.DeviceMemory, newBuffer->m_Allocation.DeviceMemoryOffset);
        }
        else
        {
            std::cout << "VulkanDeviceAllocator::Allocate failed" << std::endl;
        }
    }
    else
    {
        VkMemoryAllocateInfo allocInfo;
        ZERO_STRUCT(&allocInfo);
        
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(newBuffer->m_PhysicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(newBuffer->m_Device, &allocInfo, nullptr, &newBuffer->m_DeviceMemory);
        if (result != VK_SUCCESS)
        {
            std::cout << "vkAllocateMemory failed. Error: " << result << std::endl;
        }
        else
        {
            vkBindBufferMemory(newBuffer->m_Device, newBuffer->m_Buffer, newBuffer->m_DeviceMemory, 0);
            std::cout << "Allocated " << memRequirements.size << " bytes" << std::endl;
        }
    }
    
    return newBuffer;
}

Buffer::Buffer(VkDevice device, VkPhysicalDevice physicalDevice, VulkanDeviceAllocator* pAllocator)
	: m_Device(device)
	, m_PhysicalDevice(physicalDevice)
	, m_pAllocator(pAllocator)
	, m_Buffer(VK_NULL_HANDLE)
	, m_DeviceMemory(VK_NULL_HANDLE)
	, m_SizeInBytes(0)
	, m_Allocation()
{
}

Buffer::~Buffer()
{
	if (m_Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_Device, m_Buffer, nullptr);
		m_Buffer = VK_NULL_HANDLE;
	}

	if (m_pAllocator)
	{
		m_pAllocator->Deallocate(m_Allocation);
	}
	else
	{
		if (m_DeviceMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
			m_DeviceMemory = VK_NULL_HANDLE;
		}
	}
}

void* Buffer::Map()
{
	void* pResult = nullptr;
	if (m_pAllocator)
	{
		pResult = (void*)m_Allocation.pHostMemory;
	}
	else
	{
		VkResult result = vkMapMemory(m_Device, m_DeviceMemory, 0, m_SizeInBytes, 0, &pResult);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkMapMemory failed. Error: " << result << std::endl;
		}
	}
	
	return pResult;
}
