#pragma once
#include "Core.h"

struct DescriptorPoolParams
{
	uint32_t NumUniformBuffers = 5;
	uint32_t MaxSets = 5;
};

class VulkanDescriptorPool
{
public:
	inline VulkanDescriptorPool(VkDevice device, const DescriptorPoolParams& params)
		: m_Device(device)
		, m_Pool(VK_NULL_HANDLE)
	{
		Init(params);
	}
	
	inline ~VulkanDescriptorPool()
	{
		vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
	}
	
	inline void Init(const DescriptorPoolParams& params)
	{
		constexpr uint32_t numPoolSizes = 1;
		VkDescriptorPoolSize poolSizes[numPoolSizes];
		poolSizes[0].type 				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount 	= params.NumUniformBuffers;
		
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType 				= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount 		= numPoolSizes;
		poolInfo.pPoolSizes 		= poolSizes;
		poolInfo.maxSets 			= params.MaxSets;
		
		if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_Pool) != VK_SUCCESS)
		{
			std::cout << "Failed to create descriptor pool" << std::endl;
		}
	}
	
private:
	VkDevice m_Device;
	VkDescriptorPool m_Pool;
};
