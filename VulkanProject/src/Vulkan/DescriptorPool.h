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

    VkDescriptorPool GetPool() const
    {
        return m_Pool;
    }

private:
    VkDescriptorPool      m_Pool;
    FDescriptorPoolParams m_Params;
};
