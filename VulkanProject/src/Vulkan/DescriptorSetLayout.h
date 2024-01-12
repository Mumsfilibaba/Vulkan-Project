#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FDevice;

struct FDescriptorSetLayoutParams
{
    VkDescriptorSetLayoutBinding* pBindings   = nullptr;
    uint32_t                      numBindings = 0;
};

class FDescriptorSetLayout
{
public:
    static FDescriptorSetLayout* Create(FDevice* pDevice, const FDescriptorSetLayoutParams& params);

    FDescriptorSetLayout(VkDevice device);
    ~FDescriptorSetLayout();

    VkDescriptorSetLayout GetDescriptorSetLayout() const
    {
        return m_DescriptorSetLayout;
    }

private:
    VkDevice              m_Device;
    VkDescriptorSetLayout m_DescriptorSetLayout;
};
