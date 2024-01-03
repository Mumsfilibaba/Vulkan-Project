#include "Buffer.h"
#include "VulkanHelper.h"
#include "VulkanContext.h"

Buffer* Buffer::Create(VulkanContext* pContext, const BufferParams& params, VulkanDeviceAllocator* pAllocator)
{
    Buffer* pBuffer = new Buffer(pContext, pAllocator);
    
    VkBufferCreateInfo bufferInfo;
    ZERO_STRUCT(&bufferInfo);
    
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = params.Size;
    bufferInfo.usage       = params.Usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(pContext->GetDevice(), &bufferInfo, nullptr, &pBuffer->m_Buffer);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateBuffer failed. Error: " << result << "\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created Buffer\n";
    }

    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements(pContext->GetDevice(), pBuffer->m_Buffer, &memoryRequirements);
    
    if (pAllocator)
    {
        if (pAllocator->Allocate(pBuffer->m_Allocation, memoryRequirements, params.MemoryProperties))
        {
            vkBindBufferMemory(pContext->GetDevice(), pBuffer->m_Buffer, pBuffer->m_Allocation.DeviceMemory, pBuffer->m_Allocation.DeviceMemoryOffset);
            pBuffer->m_Size = params.Size;
        }
        else
        {
            std::cout << "VulkanDeviceAllocator::Allocate failed\n";
        }
    }
    else
    {
        VkMemoryAllocateInfo allocInfo;
        ZERO_STRUCT(&allocInfo);
        
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memoryRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(pContext->GetPhysicalDevice(), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(pContext->GetDevice(), &allocInfo, nullptr, &pBuffer->m_DeviceMemory);
        if (result != VK_SUCCESS)
        {
            std::cout << "vkAllocateMemory failed. Error: " << result << "\n";
        }
        else
        {
            vkBindBufferMemory(pContext->GetDevice(), pBuffer->m_Buffer, pBuffer->m_DeviceMemory, 0);
            pBuffer->m_Size = memoryRequirements.size;

            std::cout << "Allocated " << memoryRequirements.size << " bytes\n";
        }
    }
    
    return pBuffer;
}

Buffer* Buffer::CreateWithData(VulkanContext* pContext, const BufferParams& params, VulkanDeviceAllocator* pAllocator, const void* pSource)
{
    assert(params.MemoryProperties == VK_CPU_BUFFER_USAGE);
    
    Buffer* pBuffer = Buffer::Create(pContext, params, pAllocator);
    if (!pBuffer)
    {
        return nullptr;
    }
    
    if (pSource)
    {
        void* pData = pBuffer->Map();
        memcpy(pData, pSource, params.Size);
    
        pBuffer->FlushMappedMemoryRange();
		pBuffer->Unmap();
    }
    
    return pBuffer;
}

Buffer::Buffer(VulkanContext* pContext, VulkanDeviceAllocator* pAllocator)
	: m_pContext(pContext)
	, m_pAllocator(pAllocator)
	, m_Buffer(VK_NULL_HANDLE)
	, m_DeviceMemory(VK_NULL_HANDLE)
	, m_Size(0)
	, m_Allocation()
{
}

Buffer::~Buffer()
{
	if (m_Buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_pContext->GetDevice(), m_Buffer, nullptr);
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
			vkFreeMemory(m_pContext->GetDevice(), m_DeviceMemory, nullptr);
			m_DeviceMemory = VK_NULL_HANDLE;
		}
	}
    
    m_pContext = nullptr;
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
		VkResult result = vkMapMemory(m_pContext->GetDevice(), m_DeviceMemory, 0, m_Size, 0, &pResult);
		if (result != VK_SUCCESS)
		{
			std::cout << "vkMapMemory failed. Error: " << result << "\n";
            assert(false);
		}
	}
	
	return pResult;
}

void Buffer::FlushMappedMemoryRange()
{
    if (!m_pAllocator)
    {
        VkMappedMemoryRange range = {};
        ZERO_STRUCT(&range);

        range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = m_DeviceMemory;
        range.offset = 0;
        range.size   = m_Size;

        VkResult result = vkFlushMappedMemoryRanges(m_pContext->GetDevice(), 1, &range);
        if (result != VK_SUCCESS)
        {
            std::cout << "vkFlushMappedMemoryRanges failed. Error: " << result << "\n";
            assert(false);
        }
    }
}

void Buffer::Unmap()
{
    if (!m_pAllocator)
    {
        vkUnmapMemory(m_pContext->GetDevice(), m_DeviceMemory);
    }
}
