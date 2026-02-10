#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class CDevice;
class CDescriptorSetLayout;

struct SPipelineLayoutParams
{
    CDescriptorSetLayout** ppLayouts = nullptr;
    uint32_t NumLayouts       = 0;
    uint32_t NumPushConstants = 0;
    bool     bEnableBindless  = false;
};

class CPipelineLayout : public CDeviceChild
{
public:
    static CPipelineLayout* Create(CDevice* pDevice, const SPipelineLayoutParams& Params);

    CPipelineLayout(CDevice* pDevice);
    ~CPipelineLayout();

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
