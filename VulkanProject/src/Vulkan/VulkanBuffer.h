#pragma once
#include "Core.h"
#include "VulkanDeviceAllocator.h"

struct BufferParams
{
	VkDeviceSize SizeInBytes = 0;
	VkMemoryPropertyFlags MemoryProperties = 0;
	uint32_t Usage = 0;
};

class VulkanDeviceAllocator;

class VulkanBuffer
{
public:
	VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferParams& params, VulkanDeviceAllocator* pAllocator);
	~VulkanBuffer();

	//Not available for GPU buffers
	void Map(void** ppCPUMem);
	void Unmap();

	inline VkBuffer GetBuffer() const
	{
		return m_Buffer;
	}
	
private:
	void Init(const BufferParams& params);
	
private:
	VulkanDeviceAllocator* m_pAllocator;
	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	VkBuffer m_Buffer;
	VkDeviceMemory m_DeviceMemory;
	VkDeviceSize m_SizeInBytes;
	VkDeviceAllocation m_Allocation;
};
