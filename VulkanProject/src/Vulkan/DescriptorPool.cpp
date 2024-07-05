#include "DescriptorPool.h"
#include "Device.h"

FDescriptorPool* FDescriptorPool::Create(FDevice* pDevice, const FDescriptorPoolParams& Params)
{
    constexpr uint32_t NumPoolSizes = 4;
    
    FDescriptorPool* pDescriptorPool = new FDescriptorPool(pDevice);
    
    uint32_t NumPools = 0;
    VkDescriptorPoolSize PoolSizes[NumPoolSizes];
    if (Params.NumUniformBuffers > 0)
    {
        PoolSizes[NumPools].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        PoolSizes[NumPools].descriptorCount = Params.NumUniformBuffers;
        NumPools++;
    }
    
    if (Params.NumStorageImages > 0)
    {
        PoolSizes[NumPools].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        PoolSizes[NumPools].descriptorCount = Params.NumStorageImages;
        NumPools++;
    }
    
    if (Params.NumCombinedImageSamplers > 0)
    {
        PoolSizes[NumPools].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        PoolSizes[NumPools].descriptorCount = Params.NumCombinedImageSamplers;
        NumPools++;
    }

    if (Params.NumStorageBuffers > 0)
    {
        PoolSizes[NumPools].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        PoolSizes[NumPools].descriptorCount = Params.NumStorageBuffers;
        NumPools++;
    }
    
    VkDescriptorPoolCreateInfo DescriptorPoolCreateInfo;
    ZERO_STRUCT(&DescriptorPoolCreateInfo);
    
    DescriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolCreateInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    DescriptorPoolCreateInfo.poolSizeCount = NumPools;
    DescriptorPoolCreateInfo.pPoolSizes    = PoolSizes;
    DescriptorPoolCreateInfo.maxSets       = Params.MaxSets;
    
    if (vkCreateDescriptorPool(pDevice->GetDevice(), &DescriptorPoolCreateInfo, nullptr, &pDescriptorPool->m_Pool) != VK_SUCCESS)
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

FDescriptorPool::FDescriptorPool(FDevice* pDevice)
    : FDeviceChild(pDevice)
    , m_Pool(VK_NULL_HANDLE)
{
}

FDescriptorPool::~FDescriptorPool()
{
    if (m_Pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(GetDevice()->GetDevice(), m_Pool, nullptr);
        m_Pool = VK_NULL_HANDLE;
    }
}
