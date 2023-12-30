#include "DescriptorPool.h"
#include "VulkanContext.h"

DescriptorPool* DescriptorPool::Create(VulkanContext* pContext, const DescriptorPoolParams& params)
{
    constexpr uint32_t numPoolSizes = 2;
    
    DescriptorPool* newDescriptorPool = new DescriptorPool(pContext->GetDevice());
    
    VkDescriptorPoolSize poolSizes[numPoolSizes];
    poolSizes[0].type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount     = params.NumUniformBuffers;
    
    poolSizes[1].type                 = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount     = params.NumStorageImages;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags           = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.pNext            = nullptr;
    poolInfo.poolSizeCount = numPoolSizes;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets        = params.MaxSets;
    
    if (vkCreateDescriptorPool(newDescriptorPool->m_Device, &poolInfo, nullptr, &newDescriptorPool->m_Pool) != VK_SUCCESS)
    {
        std::cout << "Failed to create descriptor pool" << std::endl;
        return nullptr;
    }
    
    return newDescriptorPool;
}
