#pragma once
#include "DeviceChild.h"
#include <vulkan/vulkan.h>

class FDevice;
class FDescriptorSetLayout;

struct FPipelineLayoutParams
{
    FDescriptorSetLayout** ppLayouts        = nullptr;
    uint32_t               numLayouts       = 0;
    uint32_t               numPushConstants = 0;
};

class FPipelineLayout : public FDeviceChild
{
public:
    static FPipelineLayout* Create(FDevice* pDevice, const FPipelineLayoutParams& params);

    FPipelineLayout(FDevice* pDevice);
    ~FPipelineLayout();

    VkPipelineLayout GetPipelineLayout() const
    {
        return m_PipelineLayout;
    }

private:
    VkPipelineLayout m_PipelineLayout;
};
