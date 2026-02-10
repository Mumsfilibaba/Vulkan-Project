#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

struct SDescriptorSetLayoutParams
{
    VkDescriptorSetLayoutBinding* pBindings = nullptr;
    uint32_t NumBindings = 0;
};

class CDescriptorSetLayout : public CDeviceChild
{
public:
    static CDescriptorSetLayout* Create(CDevice* pDevice, const SDescriptorSetLayoutParams& Params);

    CDescriptorSetLayout(CDevice* pDevice);
    ~CDescriptorSetLayout();

    void SetDebugName(const char* DebugName);
    
    VkDescriptorSetLayout GetDescriptorSetLayout() const
    {
        return m_DescriptorSetLayout;
    }

private:
    VkDescriptorSetLayout m_DescriptorSetLayout;
};
