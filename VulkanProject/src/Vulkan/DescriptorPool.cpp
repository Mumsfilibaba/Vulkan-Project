#include "DescriptorPool.h"
#include "Device.h"
#include "Extensions.h"

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
    
    if (vkCreateDescriptorPool(pDevice->GetDevice(), &DescriptorPoolCreateInfo, nullptr, &pDescriptorPool->m_DescriptorPool) != VK_SUCCESS)
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
    , m_DescriptorPool(VK_NULL_HANDLE)
{
}

FDescriptorPool::~FDescriptorPool()
{
    if (m_DescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(GetDevice()->GetDevice(), m_DescriptorPool, nullptr);
        m_DescriptorPool = VK_NULL_HANDLE;
    }
}

void FDescriptorPool::SetDebugName(const char* DebugName)
{
    if (FExtensions::vkSetDebugUtilsObjectNameEXT)
    {
        VkDebugUtilsObjectNameInfoEXT DebugNameInfo;
        ZERO_STRUCT(&DebugNameInfo);

        DebugNameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        DebugNameInfo.objectType   = VK_OBJECT_TYPE_DESCRIPTOR_POOL;
        DebugNameInfo.pObjectName  = DebugName;
        DebugNameInfo.objectHandle = reinterpret_cast<uint64_t>(m_DescriptorPool);

        VkResult Result = FExtensions::vkSetDebugUtilsObjectNameEXT(GetDevice()->GetDevice(), &DebugNameInfo);
        if (Result != VK_SUCCESS)
        {
            std::cout << "Failed to set name '" << DebugNameInfo.pObjectName << "'.Error: " << Result << std::endl;
        }
    }
}
