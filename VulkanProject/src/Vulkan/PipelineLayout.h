#pragma once
#include "Core.h"
#include <vulkan/vulkan.h>

class FDevice;
class FDescriptorSetLayout;

struct FPipelineLayoutParams
{
    FDescriptorSetLayout** ppLayouts        = nullptr;
    uint32_t               numLayouts       = 0;
    uint32_t               numPushConstants = 0;
};

class FPipelineLayout
{
public:
    static FPipelineLayout* Create(FDevice* pDevice, const FPipelineLayoutParams& params);

    FPipelineLayout(VkDevice device);
    ~FPipelineLayout();

    VkPipelineLayout GetPipelineLayout() const
    {
        return m_PipelineLayout;
    }

private:
    VkDevice         m_Device;
    VkPipelineLayout m_PipelineLayout;
};
