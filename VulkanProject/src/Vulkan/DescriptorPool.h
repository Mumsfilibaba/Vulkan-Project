#pragma once
#include "Core.h"

struct FDescriptorPoolParams
{
    uint32_t NumUniformBuffers        = 0;
    uint32_t NumStorageImages         = 0;
    uint32_t NumCombinedImageSamplers = 0;
    uint32_t NumStorageBuffers        = 0;
    uint32_t MaxSets                  = 0;
};

class FDescriptorPool
{
public:
    static FDescriptorPool* Create(class FDevice* pDevice, const FDescriptorPoolParams& params);

    FDescriptorPool(VkDevice device);
    ~FDescriptorPool();
    
    VkDescriptorPool GetPool() const
    {
        return m_Pool;
    }
        
private:
    VkDevice              m_Device;
    VkDescriptorPool      m_Pool;
    FDescriptorPoolParams m_Params;
};
