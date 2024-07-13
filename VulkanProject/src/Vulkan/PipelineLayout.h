#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class FDevice;
class FDescriptorSetLayout;

struct FPipelineLayoutParams
{
    FDescriptorSetLayout** ppLayouts = nullptr;
    uint32_t NumLayouts       = 0;
    uint32_t NumPushConstants = 0;
    bool     bEnableBindless  = false;
};

class FPipelineLayout : public FDeviceChild
{
public:
    static FPipelineLayout* Create(FDevice* pDevice, const FPipelineLayoutParams& Params);

    FPipelineLayout(FDevice* pDevice);
    ~FPipelineLayout();

    void SetDebugName(const char* DebugName);
    
    VkPipelineLayout GetPipelineLayout() const
    {
        return m_PipelineLayout;
    }
    
    uint32_t GetBindlessDescriptorSetIndex() const
    {
        return m_BindlessDescriptorSetIndex;
    }

private:
    VkPipelineLayout m_PipelineLayout;
    uint32_t         m_BindlessDescriptorSetIndex;
};
