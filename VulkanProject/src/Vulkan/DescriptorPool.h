#pragma once
#include "Core.h"

struct DescriptorPoolParams
{
    uint32_t NumUniformBuffers        = 0;
    uint32_t NumStorageImages         = 0;
    uint32_t NumCombinedImageSamplers = 0;
    uint32_t NumStorageBuffers        = 0;
    uint32_t MaxSets                  = 0;
};

class DescriptorPool
{
public:
    static DescriptorPool* Create(class Device* pDevice, const DescriptorPoolParams& params);

    DescriptorPool(VkDevice device);
    ~DescriptorPool();
    
    VkDescriptorPool GetPool() const
    {
        return m_Pool;
    }
        
private:
    VkDevice             m_Device;
    VkDescriptorPool     m_Pool;
    DescriptorPoolParams m_Params;
};
