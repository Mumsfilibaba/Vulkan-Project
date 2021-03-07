#pragma once
#include "Core.h"

struct DescriptorPoolParams
{
	uint32_t NumUniformBuffers = 5;
	uint32_t NumStorageImages  = 5;
	uint32_t MaxSets 		   = 5;
};

class DescriptorPool
{
public:
	inline DescriptorPool(VkDevice device)
		: m_Device(device)
		, m_Pool(VK_NULL_HANDLE)
	{
	}
	
	inline ~DescriptorPool()
	{
		vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
	}
	
	inline VkDescriptorPool GetPool() const
	{
		return m_Pool;
	}
	
	static DescriptorPool* Create(class VulkanContext* pContext, const DescriptorPoolParams& params);
	
private:
	VkDevice m_Device;
	VkDescriptorPool m_Pool;
};
