#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

struct FDescriptorSetLayoutParams
{
    VkDescriptorSetLayoutBinding* pBindings = nullptr;
    uint32_t NumBindings = 0;
};

class FDescriptorSetLayout : public FDeviceChild
{
public:
    static FDescriptorSetLayout* Create(FDevice* pDevice, const FDescriptorSetLayoutParams& Params);

    FDescriptorSetLayout(FDevice* pDevice);
    ~FDescriptorSetLayout();

    VkDescriptorSetLayout GetDescriptorSetLayout() const
    {
        return m_DescriptorSetLayout;
    }

private:
    VkDescriptorSetLayout m_DescriptorSetLayout;
};
