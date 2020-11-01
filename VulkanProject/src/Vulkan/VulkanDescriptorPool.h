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
	VulkanDescriptorPool();
	~VulkanDescriptorPool();
	
	inline void Init(const DescriptorPoolParams& params)
	{
		constexpr uint32_t numPoolSizes = 1;
		VkDescriptorPoolSize poolSizes[numPoolSizes];
		poolSize.type 				= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount 	= params.NumUniformBuffers;
		
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType 				= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount 		= numPoolSizes;
		poolInfo.pPoolSizes 		= poolSizes;
		poolInfo.maxSets 			= params.MaxSets;
		
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			std::cout << "Failed to create descriptor pool" << std::endl;
		}
	}
	
private:
	VkDescriptorPool m_Pool;
};
