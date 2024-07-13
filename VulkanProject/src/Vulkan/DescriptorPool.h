#pragma once
#include "DeviceChild.h"

struct FDescriptorPoolParams
{
    uint32_t NumUniformBuffers        = 0;
    uint32_t NumStorageImages         = 0;
    uint32_t NumCombinedImageSamplers = 0;
    uint32_t NumStorageBuffers        = 0;
    uint32_t MaxSets                  = 0;
};

class FDescriptorPool : public FDeviceChild
{
public:
    static FDescriptorPool* Create(FDevice* pDevice, const FDescriptorPoolParams& Params);

    FDescriptorPool(FDevice* pDevice);
    ~FDescriptorPool();

    void SetDebugName(const char* DebugName);

    VkDescriptorPool GetPool() const
    {
        return m_DescriptorPool;
    }

private:
    VkDescriptorPool      m_DescriptorPool;
    FDescriptorPoolParams m_Params;
};
