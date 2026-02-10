#pragma once
#include "DeviceChild.h"

struct SDescriptorPoolParams
{
    uint32_t NumUniformBuffers        = 0;
    uint32_t NumStorageImages         = 0;
    uint32_t NumCombinedImageSamplers = 0;
    uint32_t NumStorageBuffers        = 0;
    uint32_t MaxSets                  = 0;
};

class CDescriptorPool : public CDeviceChild
{
public:
    static CDescriptorPool* Create(CDevice* pDevice, const SDescriptorPoolParams& Params);

    CDescriptorPool(CDevice* pDevice);
    ~CDescriptorPool();

    void SetDebugName(const char* DebugName);

    VkDescriptorPool GetPool() const
    {
        return m_DescriptorPool;
    }

private:
    VkDescriptorPool      m_DescriptorPool;
    SDescriptorPoolParams m_Params;
};
