#pragma once
#include "Core.h"

#include <vulkan/vulkan.h>

struct BufferParams
{
	VkDeviceSize SizeInBytes = 0;
	uint32 Usage = 0;
};

class VulkanBuffer
{
public:
	DECL_NO_COPY(VulkanBuffer);

	VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferParams& params);
	~VulkanBuffer();

	//Not available for GPU buffers
	void Map(void** ppCPUMem);
	void Unmap();

	VkBuffer GetBuffer() const { return m_Buffer; }
private:
	void Init(VkPhysicalDevice physicalDevice, const BufferParams& params);
private:
	VkDevice m_Device;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
	VkDeviceSize m_SizeInBytes;
};