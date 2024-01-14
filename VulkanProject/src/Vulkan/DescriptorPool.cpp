#include "DescriptorPool.h"
#include "Device.h"

FDescriptorPool* FDescriptorPool::Create(FDevice* pDevice, const FDescriptorPoolParams& params)
{
    constexpr uint32_t numPoolSizes = 4;
    
    FDescriptorPool* pDescriptorPool = new FDescriptorPool(pDevice->GetDevice());
    
    uint32_t numPools = 0;
    VkDescriptorPoolSize poolSizes[numPoolSizes];
    if (params.NumUniformBuffers > 0)
    {
        poolSizes[numPools].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[numPools].descriptorCount = params.NumUniformBuffers;
        numPools++;
    }
    
    if (params.NumStorageImages > 0)
    {
        poolSizes[numPools].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[numPools].descriptorCount = params.NumStorageImages;
        numPools++;
    }
    
    if (params.NumCombinedImageSamplers > 0)
    {
        poolSizes[numPools].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[numPools].descriptorCount = params.NumCombinedImageSamplers;
        numPools++;
    }

    if (params.NumStorageBuffers > 0)
    {
        poolSizes[numPools].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[numPools].descriptorCount = params.NumStorageBuffers;
        numPools++;
    }
    
    VkDescriptorPoolCreateInfo poolInfo;
    ZERO_STRUCT(&poolInfo);
    
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = numPools;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets       = params.MaxSets;
    
    if (vkCreateDescriptorPool(pDescriptorPool->m_Device, &poolInfo, nullptr, &pDescriptorPool->m_Pool) != VK_SUCCESS)
    {
        std::cout << "vkCreateDescriptorPool failed\n";
        return nullptr;
    }
    else
    {
        std::cout << "Created DescriptorPool\n";
        return pDescriptorPool;
    }
}

FDescriptorPool::FDescriptorPool(VkDevice device)
    : m_Device(device)
    , m_Pool(VK_NULL_HANDLE)
{
}

FDescriptorPool::~FDescriptorPool()
{
    if (m_Pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
        m_Pool = VK_NULL_HANDLE;
    }

    m_Device = VK_NULL_HANDLE;
}
